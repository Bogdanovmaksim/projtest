#include "task_manager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace task_manager {
namespace {

/*!
 * \brief Возвращает текущую календарную дату.
 *
 * Получает текущую системную дату, округляет её до дней
 * и преобразует в объект Date.
 *
 * \return Текущая дата в формате Date.
 */
Date currentDate() {
    const auto now = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    const std::chrono::year_month_day calendar{now};

    return Date{static_cast<int>(calendar.year()),
                static_cast<int>(static_cast<unsigned>(calendar.month())),
                static_cast<int>(static_cast<unsigned>(calendar.day()))};
}

/*!
 * \brief Проверяет, что дедлайн не находится в прошлом.
 *
 * Сравнивает переданную дату с текущей датой.
 * Если дедлайн раньше сегодняшнего дня, выбрасывает исключение.
 *
 * \param[in] deadline Проверяемая дата дедлайна.
 *
 * \throws std::invalid_argument Если дедлайн находится в прошлом.
 */
void ensureDeadlineIsNotPast(const Date& deadline) {
    if (compareDates(deadline, currentDate()) < 0) {
        throw std::invalid_argument("Дедлайн не может быть раньше сегодняшней даты");
    }
}

}  // namespace

/*!
 * \brief Создает менеджер задач.
 *
 * Инициализирует пути к основному файлу хранения задач
 * и файлу архива.
 *
 * \param[in] storagePath Путь к файлу с активными задачами.
 * \param[in] archivePath Путь к архивному файлу.
 *
 * \throws std::invalid_argument Если один из путей пустой.
 */
TaskManager::TaskManager(std::filesystem::path storagePath, std::filesystem::path archivePath)
    : storagePath_(std::move(storagePath)), archivePath_(std::move(archivePath)) {
    if (storagePath_.empty() || archivePath_.empty()) {
        throw std::invalid_argument("Пути к файлам не должны быть пустыми");
    }
}

/*!
 * \brief Загружает задачи из файла хранения.
 *
 * Очищает текущий список задач и считывает данные из файла.
 * Если файл отсутствует, менеджер начинает работу с пустым списком.
 *
 * \throws std::runtime_error Если файл не удалось открыть
 * или если в нем есть некорректная строка.
 */
void TaskManager::load() {
    tasks_.clear();

    if (!std::filesystem::exists(storagePath_)) {
        nextId_ = 1;
        return;
    }

    std::ifstream input(storagePath_);
    if (!input) {
        throw std::runtime_error("Не удалось открыть файл задач");
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

        if (line.empty()) {
            continue;
        }

        try {
            tasks_.push_back(deserializeTask(line));
        } catch (const std::exception& error) {
            std::ostringstream message;
            message << "Ошибка в строке " << lineNumber << ": " << error.what();
            throw std::runtime_error(message.str());
        }
    }

    refreshNextId();
}

/*!
 * \brief Сохраняет задачи в основной файл.
 *
 * Перезаписывает файл хранения и записывает в него
 * все текущие задачи в сериализованном виде.
 *
 * \throws std::runtime_error Если файл не удалось открыть для записи.
 */
void TaskManager::save() const {
    std::ofstream output(storagePath_, std::ios::trunc);

    if (!output) {
        throw std::runtime_error("Не удалось открыть файл задач для записи");
    }

    for (const Task& task : tasks_) {
        output << serializeTask(task) << '\n';
    }
}

/*!
 * \brief Добавляет новую задачу.
 *
 * Проверяет корректность дедлайна, создает задачу
 * с уникальным идентификатором и добавляет её в список.
 *
 * \param[in] description Описание задачи.
 * \param[in] deadline Дата дедлайна.
 * \param[in] category Категория задачи.
 * \param[in] importance Уровень важности задачи.
 *
 * \return Созданная задача.
 *
 * \throws std::invalid_argument Если дедлайн находится в прошлом.
 */
Task TaskManager::addTask(std::string description, Date deadline, std::string category,
                          Importance importance) {
    ensureDeadlineIsNotPast(deadline);

    Task task{nextId_++, std::move(description), deadline, std::move(category), importance,
              Status::Active};

    serializeTask(task);
    tasks_.push_back(task);

    return task;
}

/*!
 * \brief Изменяет поля существующей задачи.
 *
 * Находит задачу по идентификатору и изменяет только те поля,
 * которые были переданы в объекте TaskUpdate.
 *
 * \param[in] id Идентификатор изменяемой задачи.
 * \param[in] update Набор новых значений для полей задачи.
 *
 * \return Обновленная задача.
 *
 * \throws std::runtime_error Если задача с указанным идентификатором не найдена.
 * \throws std::invalid_argument Если новый дедлайн некорректен.
 */
Task TaskManager::updateTask(int id, const TaskUpdate& update) {
    Task& task = findTask(id);
    Task candidate = task;

    if (update.description.has_value()) {
        candidate.description = update.description.value();
    }

    if (update.deadline.has_value()) {
        ensureDeadlineIsNotPast(update.deadline.value());

        if (compareDates(update.deadline.value(), task.deadline) < 0) {
            throw std::invalid_argument("Новый дедлайн не может быть раньше текущего");
        }

        candidate.deadline = update.deadline.value();
    }

    if (update.category.has_value()) {
        candidate.category = update.category.value();
    }

    if (update.importance.has_value()) {
        candidate.importance = update.importance.value();
    }

    if (update.status.has_value()) {
        candidate.status = update.status.value();
    }

    serializeTask(candidate);
    task = candidate;

    return task;
}

/*!
 * \brief Отмечает задачу выполненной.
 *
 * Изменяет статус задачи на Done.
 *
 * \param[in] id Идентификатор задачи.
 *
 * \throws std::runtime_error Если задача не найдена.
 */
void TaskManager::markDone(int id) {
    TaskUpdate update;
    update.status = Status::Done;

    updateTask(id, update);
}

/*!
 * \brief Удаляет задачу из списка.
 *
 * Находит задачу по идентификатору, удаляет её из коллекции
 * и возвращает удаленный объект.
 *
 * \param[in] id Идентификатор удаляемой задачи.
 *
 * \return Удаленная задача.
 *
 * \throws std::runtime_error Если задача не найдена.
 */
Task TaskManager::removeTask(int id) {
    const auto iterator = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& task) {
        return task.id == id;
    });

    if (iterator == tasks_.end()) {
        throw std::runtime_error("Задача не найдена");
    }

    Task removed = *iterator;
    tasks_.erase(iterator);

    return removed;
}

/*!
 * \brief Возвращает задачи, подходящие под фильтр.
 *
 * Проверяет каждую задачу с помощью функции taskMatchesFilter
 * и добавляет подходящие задачи в результирующий список.
 *
 * \param[in] filter Условия фильтрации задач.
 *
 * \return Список задач, соответствующих фильтру.
 */
std::vector<Task> TaskManager::filterTasks(const TaskFilter& filter) const {
    std::vector<Task> result;

    for (const Task& task : tasks_) {
        if (taskMatchesFilter(task, filter)) {
            result.push_back(task);
        }
    }

    return result;
}

/*!
 * \brief Возвращает задачи, отсортированные по дедлайну.
 *
 * Создает копию текущего списка задач и сортирует её
 * по дате дедлайна.
 *
 * \return Отсортированный список задач.
 */
std::vector<Task> TaskManager::sortedByDeadline() const {
    std::vector<Task> result = tasks_;
    sortByDeadline(result);

    return result;
}

/*!
 * \brief Переносит выполненные задачи в архив.
 *
 * Записывает все задачи со статусом Done в архивный файл,
 * после чего удаляет их из основного списка задач.
 *
 * \return Количество архивированных задач.
 *
 * \throws std::runtime_error Если архивный файл не удалось открыть.
 */
std::size_t TaskManager::archiveCompleted() {
    std::ofstream archive(archivePath_, std::ios::app);

    if (!archive) {
        throw std::runtime_error("Не удалось открыть архив");
    }

    std::size_t count = 0;

    for (const Task& task : tasks_) {
        if (task.status == Status::Done) {
            archive << serializeTask(task) << '\n';
            ++count;
        }
    }

    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(), [](const Task& task) {
                     return task.status == Status::Done;
                 }),
                 tasks_.end());

    return count;
}

/*!
 * \brief Возвращает текущий список задач.
 *
 * Предоставляет доступ только для чтения к внутреннему
 * контейнеру задач.
 *
 * \return Константная ссылка на список задач.
 */
const std::vector<Task>& TaskManager::tasks() const {
    return tasks_;
}

/*!
 * \brief Находит задачу по идентификатору.
 *
 * Выполняет поиск задачи в списке и возвращает изменяемую ссылку
 * на найденный объект.
 *
 * \param[in] id Идентификатор задачи.
 *
 * \return Ссылка на найденную задачу.
 *
 * \throws std::runtime_error Если задача не найдена.
 */
Task& TaskManager::findTask(int id) {
    const auto iterator = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& task) {
        return task.id == id;
    });

    if (iterator == tasks_.end()) {
        throw std::runtime_error("Задача не найдена");
    }

    return *iterator;
}

/*!
 * \brief Находит задачу по идентификатору без возможности изменения.
 *
 * Выполняет поиск задачи в списке и возвращает константную ссылку
 * на найденный объект.
 *
 * \param[in] id Идентификатор задачи.
 *
 * \return Константная ссылка на найденную задачу.
 *
 * \throws std::runtime_error Если задача не найдена.
 */
const Task& TaskManager::findTask(int id) const {
    const auto iterator = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& task) {
        return task.id == id;
    });

    if (iterator == tasks_.end()) {
        throw std::runtime_error("Задача не найдена");
    }

    return *iterator;
}

/*!
 * \brief Пересчитывает следующий свободный идентификатор.
 *
 * Используется после загрузки задач из файла.
 * Метод ищет максимальный существующий идентификатор
 * и устанавливает следующий идентификатор на единицу больше.
 */
void TaskManager::refreshNextId() {
    nextId_ = 1;

    for (const Task& task : tasks_) {
        if (task.id >= nextId_) {
            nextId_ = task.id + 1;
        }
    }
}

}  // namespace task_manager