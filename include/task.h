#pragma once

#include <optional>
#include <string>
#include <vector>

namespace task_manager {

/*!
 * \brief Дата дедлайна задачи.
 */
struct Date {
    int year{};  ///< Год.
    int month{}; ///< Месяц.
    int day{};   ///< День.
};

/*!
 * \brief Уровень важности задачи.
 */
enum class Importance {
    Low,    ///< Низкая важность.
    Medium, ///< Средняя важность.
    High    ///< Высокая важность.
};

/*!
 * \brief Статус задачи.
 */
enum class Status {
    Active, ///< Задача активна.
    Done    ///< Задача выполнена.
};

/*!
 * \brief Задача пользователя.
 */
struct Task {
    int id{};                      ///< Уникальный идентификатор.
    std::string description;       ///< Описание задачи.
    Date deadline{};               ///< Дедлайн.
    std::string category;          ///< Категория.
    Importance importance{};       ///< Уровень важности.
    Status status{Status::Active}; ///< Статус выполнения.
};

/*!
 * \brief Параметры фильтрации задач.
 */
struct TaskFilter {
    std::optional<std::string> keyword;   ///< Ключевое слово.
    std::optional<Importance> importance; ///< Фильтр по важности.
    std::optional<Status> status;         ///< Фильтр по статусу.
    std::optional<int> daysFromBaseDate;  ///< Количество ближайших дней.
    Date baseDate{};                      ///< Дата начала фильтра.
};

/*!
 * \brief Преобразует важность в строку для файла.
 * \param[in] importance Уровень важности.
 * \return Строка low, medium или high.
 */
std::string importanceToString(Importance importance);

/*!
 * \brief Преобразует строку в важность.
 * \param[in] text Текст: low, medium, high, низкая, средняя или высокая.
 * \return Уровень важности.
 * \throws std::invalid_argument Если значение неизвестно.
 */
Importance importanceFromString(const std::string& text);

/*!
 * \brief Преобразует статус в строку для файла.
 * \param[in] status Статус задачи.
 * \return Строка active или done.
 */
std::string statusToString(Status status);

/*!
 * \brief Преобразует строку в статус.
 * \param[in] text Текст: active, done, активна или выполнена.
 * \return Статус задачи.
 * \throws std::invalid_argument Если значение неизвестно.
 */
Status statusFromString(const std::string& text);

/*!
 * \brief Разбирает дату формата YYYY-MM-DD.
 * \param[in] text Строка с датой.
 * \return Распознанная дата.
 * \throws std::invalid_argument Если дата некорректна.
 */
Date parseDate(const std::string& text);

/*!
 * \brief Форматирует дату в YYYY-MM-DD.
 * \param[in] date Дата.
 * \return Строковое представление даты.
 */
std::string dateToString(const Date& date);

/*!
 * \brief Сравнивает две даты.
 * \param[in] left Первая дата.
 * \param[in] right Вторая дата.
 * \return Отрицательное число, если left раньше right; 0, если даты равны.
 */
int compareDates(const Date& left, const Date& right);

/*!
 * \brief Считает расстояние между датами.
 * \param[in] from Начальная дата.
 * \param[in] to Конечная дата.
 * \return Количество дней между датами.
 */
int daysBetween(const Date& from, const Date& to);

/*!
 * \brief Преобразует задачу в строку файла.
 * \param[in] task Задача.
 * \return Строка для сохранения.
 */
std::string serializeTask(const Task& task);

/*!
 * \brief Восстанавливает задачу из строки файла.
 * \param[in] line Строка файла.
 * \return Задача.
 */
Task deserializeTask(const std::string& line);

/*!
 * \brief Проверяет соответствие задачи фильтру.
 * \param[in] task Задача.
 * \param[in] filter Фильтр.
 * \return true, если задача подходит.
 */
bool taskMatchesFilter(const Task& task, const TaskFilter& filter);

/*!
 * \brief Сортирует задачи по приближению дедлайна.
 * \param[in,out] tasks Список задач.
 */
void sortByDeadline(std::vector<Task>& tasks);

} // namespace task_manager
