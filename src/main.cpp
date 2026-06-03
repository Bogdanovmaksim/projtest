#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "task_manager.h"

using task_manager::Date;
using task_manager::Importance;
using task_manager::Status;
using task_manager::Task;
using task_manager::TaskFilter;
using task_manager::TaskManager;
using task_manager::TaskUpdate;

namespace {

/*!
 * \brief Считывает строку из консоли.
 * \param[in] prompt Приглашение, которое выводится перед вводом.
 * \return Строка, введенная пользователем.
 * \throws std::runtime_error Если поток ввода закрыт.
 */
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value)) {
        throw std::runtime_error("Поток ввода закрыт");
    }
    return value;
}

/*!
 * \brief Считывает целое число.
 * \param[in] prompt Приглашение, которое выводится перед вводом.
 * \return Целое число, введенное пользователем.
 * \throws std::invalid_argument Если пользователь ввел не целое число.
 */
int readInt(const std::string& prompt) {
    const std::string text = readLine(prompt);
    std::size_t position = 0;
    const int value = std::stoi(text, &position);
    if (position != text.size()) {
        throw std::invalid_argument("Нужно ввести целое число");
    }
    return value;
}

/*!
 * \brief Возвращает текущую дату.
 * \return Текущая календарная дата.
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
 * \param[in] deadline Дата дедлайна для проверки.
 * \throws std::invalid_argument Если дедлайн раньше сегодняшней даты.
 */
void ensureDeadlineIsNotPast(const Date& deadline) {
    if (task_manager::compareDates(deadline, currentDate()) < 0) {
        throw std::invalid_argument("Дедлайн не может быть раньше сегодняшней даты");
    }
}

/*!
 * \brief Считывает путь или значение по умолчанию.
 * \param[in] prompt Название запрашиваемого пути.
 * \param[in] defaultValue Значение, используемое при пустом вводе.
 * \return Путь, выбранный пользователем.
 */
std::filesystem::path readPathOrDefault(const std::string& prompt, const std::string& defaultValue) {
    const std::string value = readLine(prompt + " [" + defaultValue + "]: ");
    return value.empty() ? defaultValue : value;
}

/*!
 * \brief Возвращает русское название важности.
 * \param[in] importance Значение важности задачи.
 * \return Название важности для вывода пользователю.
 * \throws std::invalid_argument Если значение важности неизвестно.
 */
std::string importanceToRussian(Importance importance) {
    switch (importance) {
    case Importance::Low:
        return "низкая";
    case Importance::Medium:
        return "средняя";
    case Importance::High:
        return "высокая";
    }
    throw std::invalid_argument("Неизвестная важность");
}

/*!
 * \brief Возвращает русское название статуса.
 * \param[in] status Статус задачи.
 * \return Название статуса для вывода пользователю.
 * \throws std::invalid_argument Если значение статуса неизвестно.
 */
std::string statusToRussian(Status status) {
    switch (status) {
    case Status::Active:
        return "активна";
    case Status::Done:
        return "выполнена";
    }
    throw std::invalid_argument("Неизвестный статус");
}

/*!
 * \brief Считывает важность задачи.
 * \return Уровень важности задачи.
 * \throws std::invalid_argument Если введенное значение важности неизвестно.
 */
Importance readImportance() {
    std::cout << "Важность: низкая / средняя / высокая\n";
    return task_manager::importanceFromString(readLine("Ввод: "));
}

/*!
 * \brief Удаляет символ ошибки кодировки из текста.
 * \param[in] text Исходный текст.
 * \return Текст без служебных символов и символа ошибки кодировки.
 */
std::string cleanConsoleText(const std::string& text) {
    std::string result;
    for (std::size_t index = 0; index < text.size();) {
        const bool replacement = index + 2 < text.size() &&
                                 static_cast<unsigned char>(text[index]) == 0xEF &&
                                 static_cast<unsigned char>(text[index + 1]) == 0xBF &&
                                 static_cast<unsigned char>(text[index + 2]) == 0xBD;
        if (replacement) {
            index += 3;
        } else if (text[index] == '\n' || text[index] == '\r' || text[index] == '\t') {
            result.push_back(' ');
            ++index;
        } else {
            result.push_back(text[index]);
            ++index;
        }
    }
    return result;
}

/*!
 * \brief Спрашивает, вернуться ли в меню.
 * \return true, если пользователь хочет продолжить работу; false, если выбран выход.
 */
bool waitForNextAction() {
    std::cout << "\nEnter - вернуться в меню, 0 - сохранить и выйти: ";
    std::string answer;
    std::getline(std::cin, answer);
    return answer != "0";
}

/*!
 * \brief Печатает одну задачу.
 * \param[in] task Задача для вывода.
 */
void printTask(const Task& task) {
    std::cout << "#" << task.id << '\n'
              << "  Дедлайн: " << task_manager::dateToString(task.deadline) << '\n'
              << "  Важность: " << importanceToRussian(task.importance) << '\n'
              << "  Статус: " << statusToRussian(task.status) << '\n'
              << "  Категория: " << cleanConsoleText(task.category) << '\n'
              << "  Описание: " << cleanConsoleText(task.description) << "\n\n";
}

/*!
 * \brief Печатает список задач.
 * \param[in] tasks Список задач для вывода.
 * \param[in] title Заголовок списка.
 */
void printTaskList(const std::vector<Task>& tasks, const std::string& title) {
    std::cout << "\n" << title << "\n";
    std::cout << "Найдено задач: " << tasks.size() << "\n";
    if (tasks.empty()) {
        std::cout << "Задач пока нет. Добавьте первую задачу через пункт 1.\n";
        return;
    }
    for (const Task& task : tasks) {
        printTask(task);
    }
}

/*!
 * \brief Добавляет задачу через консоль.
 * \param[in,out] manager Менеджер задач, в который добавляется новая задача.
 * \throws std::invalid_argument Если введенные данные некорректны.
 * \throws std::runtime_error Если сохранение в файл невозможно.
 */
void addTask(TaskManager& manager) {
    const std::string description = cleanConsoleText(readLine("Описание: "));
    const Date deadline = task_manager::parseDate(readLine("Дедлайн (ДД.ММ.ГГГГ): "));
    ensureDeadlineIsNotPast(deadline);
    const std::string category = cleanConsoleText(readLine("Категория: "));
    const Importance importance = readImportance();
    const Task task = manager.addTask(description, deadline, category, importance);
    manager.save();
    std::cout << "Добавлена задача #" << task.id << ".\n";
}

/*!
 * \brief Изменяет задачу через консоль.
 * \param[in,out] manager Менеджер задач, в котором изменяется задача.
 * \throws std::invalid_argument Если введенные данные некорректны.
 * \throws std::runtime_error Если задача не найдена или сохранение невозможно.
 */
void editTask(TaskManager& manager) {
    const int id = readInt("ID задачи: ");
    TaskUpdate update;
    const std::string description = readLine("Новое описание (Enter - без изменений): ");
    if (!description.empty()) {
        update.description = cleanConsoleText(description);
    }
    const std::string deadline = readLine("Новый дедлайн ДД.ММ.ГГГГ (Enter - без изменений): ");
    if (!deadline.empty()) {
        const Date parsedDeadline = task_manager::parseDate(deadline);
        ensureDeadlineIsNotPast(parsedDeadline);
        update.deadline = parsedDeadline;
    }
    const std::string category = readLine("Новая категория (Enter - без изменений): ");
    if (!category.empty()) {
        update.category = cleanConsoleText(category);
    }
    const std::string importance =
        readLine("Новая важность низкая/средняя/высокая (Enter - без изменений): ");
    if (!importance.empty()) {
        update.importance = task_manager::importanceFromString(importance);
    }
    const std::string status = readLine("Новый статус активна/выполнена (Enter - без изменений): ");
    if (!status.empty()) {
        update.status = task_manager::statusFromString(status);
    }
    printTask(manager.updateTask(id, update));
    manager.save();
}

/*!
 * \brief Отмечает задачу выполненной.
 * \param[in,out] manager Менеджер задач, в котором изменяется статус.
 * \throws std::runtime_error Если задача не найдена или сохранение невозможно.
 */
void markDone(TaskManager& manager) {
    manager.markDone(readInt("ID задачи: "));
    manager.save();
    std::cout << "Задача отмечена выполненной.\n";
}

/*!
 * \brief Удаляет задачу.
 * \param[in,out] manager Менеджер задач, из которого удаляется задача.
 * \throws std::runtime_error Если задача не найдена или сохранение невозможно.
 */
void removeTask(TaskManager& manager) {
    const Task removed = manager.removeTask(readInt("ID задачи: "));
    manager.save();
    std::cout << "Удалена задача:\n";
    printTask(removed);
}

/*!
 * \brief Фильтрует задачи.
 * \param[in] manager Менеджер задач, из которого берутся данные.
 * \throws std::invalid_argument Если параметры фильтра некорректны.
 */
void filterTasks(const TaskManager& manager) {
    TaskFilter filter;
    const std::string keyword = readLine("Ключевое слово (Enter - пропустить): ");
    if (!keyword.empty()) {
        filter.keyword = keyword;
    }
    const std::string importance =
        readLine("Фильтр по важности низкая/средняя/высокая (Enter - пропустить): ");
    if (!importance.empty()) {
        filter.importance = task_manager::importanceFromString(importance);
    }
    const std::string status = readLine("Фильтр по статусу активна/выполнена (Enter - пропустить): ");
    if (!status.empty()) {
        filter.status = task_manager::statusFromString(status);
    }
    const std::string days = readLine("Ближайшие N дней (Enter - пропустить): ");
    if (!days.empty()) {
        filter.daysFromBaseDate = std::stoi(days);
        filter.baseDate = currentDate();
    }
    printTaskList(manager.filterTasks(filter), "Результаты фильтрации");
}

/*!
 * \brief Архивирует выполненные задачи.
 * \param[in,out] manager Менеджер задач, который выполняет архивацию.
 * \throws std::runtime_error Если файл архива невозможно открыть.
 */
void archiveCompleted(TaskManager& manager) {
    const std::size_t count = manager.archiveCompleted();
    manager.save();
    std::cout << "Архивировано задач: " << count << ".\n";
}

/*!
 * \brief Печатает меню.
 */
void printMenu() {
    std::cout << "\nМенеджер задач\n"
              << "1. Добавить задачу\n"
              << "2. Показать все задачи\n"
              << "3. Фильтровать задачи\n"
              << "4. Показать задачи по дедлайну\n"
              << "5. Отметить выполненной\n"
              << "6. Удалить задачу\n"
              << "7. Изменить задачу\n"
              << "8. Архивировать выполненные\n"
              << "0. Сохранить и выйти\n";
}

/*!
 * \brief Выполняет команду меню.
 * \param[in,out] manager Менеджер задач.
 * \param[in] command Номер команды меню.
 * \throws std::invalid_argument Если номер команды неизвестен.
 */
void handleCommand(TaskManager& manager, int command) {
    switch (command) {
    case 1:
        addTask(manager);
        break;
    case 2:
        printTaskList(manager.tasks(), "Все задачи");
        break;
    case 3:
        filterTasks(manager);
        break;
    case 4:
        printTaskList(manager.sortedByDeadline(), "Задачи по дедлайну");
        break;
    case 5:
        markDone(manager);
        break;
    case 6:
        removeTask(manager);
        break;
    case 7:
        editTask(manager);
        break;
    case 8:
        archiveCompleted(manager);
        break;
    default:
        throw std::invalid_argument("Неизвестная команда меню");
    }
}

} // namespace

int main() {
    try {
        const auto storagePath = readPathOrDefault("Файл задач", "tasks.txt");
        const auto archivePath = readPathOrDefault("Файл архива", "archive.txt");
        TaskManager manager(storagePath, archivePath);
        manager.load();

        while (true) {
            try {
                printMenu();
                const int command = readInt("Выбор (номер + Enter): ");
                if (command == 0) {
                    manager.save();
                    std::cout << "Данные сохранены. Выход из программы.\n";
                    return 0;
                }
                handleCommand(manager, command);
                if (!waitForNextAction()) {
                    manager.save();
                    std::cout << "Данные сохранены. Выход из программы.\n";
                    return 0;
                }
            } catch (const std::exception& error) {
                if (std::cin.eof()) {
                    manager.save();
                    return 0;
                }
                std::cout << "Ошибка: " << error.what() << '\n';
            } catch (...) {
                std::cout << "Ошибка: неизвестное исключение\n";
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Критическая ошибка: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Критическая ошибка: неизвестное исключение\n";
        return 1;
    }
}
