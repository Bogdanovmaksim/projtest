#include "task.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace task_manager {
namespace {

/*!
\brief Удаляет пробелы по краям строки.
\param[in] text Исходная строка.
\return Строка без начальных и конечных пробельных символов.
*/
std::string trim(const std::string &text) {
  auto begin = text.begin();
  while (begin != text.end() && std::isspace(static_cast<unsigned char>(*begin))) {
    ++begin;
  }
  auto end = text.end();
  while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
    --end;
  }
  return std::string(begin, end);
}

/*!
\brief Переводит строку в нижний регистр.
\param[in] text Исходная строка.
\return Копия строки, приведенная к нижнему регистру.
*/
std::string toLower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return text;
}

/*!
\brief Проверяет високосный год.
\param[in] year Проверяемый год.
\return true, если год високосный, иначе false.
*/
bool isLeapYear(int year) {
  return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

/*!
\brief Возвращает число дней в месяце.
\param[in] year Год, которому принадлежит месяц.
\param[in] month Номер месяца от 1 до 12.
\return Количество дней в указанном месяце.
*/
int daysInMonth(int year, int month) {
  if (month < 1 || month > 12) {
    throw std::invalid_argument("Месяц должен быть от 1 до 12");
  }
  const std::vector<int> days{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return days.at(static_cast<std::size_t>(month - 1));
}

/*!
\brief Проверяет корректность даты.
\param[in] date Дата для проверки.
*/
void validateDate(const Date &date) {
  if (date.year < 1900 || date.year > 3000) {
    throw std::invalid_argument("Год должен быть от 1900 до 3000");
  }
  if (date.month < 1 || date.month > 12) {
    throw std::invalid_argument("Месяц должен быть от 1 до 12");
  }
  if (date.day < 1 || date.day > daysInMonth(date.year, date.month)) {
    throw std::invalid_argument("Некорректный день месяца");
  }
}

/*!
\brief Преобразует дату в chrono.
\param[in] date Дата проекта.
\return Дата в формате std::chrono::sys_days.
*/
std::chrono::sys_days toSysDays(const Date &date) {
  validateDate(date);
  return std::chrono::sys_days{std::chrono::year{date.year} /
                               std::chrono::month{static_cast<unsigned>(date.month)} /
                               std::chrono::day{static_cast<unsigned>(date.day)}};
}

/*!
\brief Проверяет обязательные поля задачи.
\param[in] task Задача для проверки.
*/
void validateTask(const Task &task) {
  if (task.id <= 0) {
    throw std::invalid_argument("ID задачи должен быть положительным");
  }
  if (trim(task.description).empty()) {
    throw std::invalid_argument("Описание не должно быть пустым");
  }
  if (trim(task.category).empty()) {
    throw std::invalid_argument("Категория не должна быть пустой");
  }
  validateDate(task.deadline);
}

/*!
\brief Экранирует поле для записи в файл.
\param[in] field Исходное текстовое поле задачи.
\return Поле с экранированными служебными символами.
*/
std::string escapeField(const std::string &field) {
  std::string result;
  for (char character : field) {
    if (character == '\\' || character == '|') {
      result.push_back('\\');
      result.push_back(character);
    } else if (character == '\n') {
      result += "\\n";
    } else if (character == '\r') {
      result += "\\r";
    } else {
      result.push_back(character);
    }
  }
  return result;
}

/*!
\brief Восстанавливает экранированное поле.
\param[in] field Экранированное поле из файла.
\return Восстановленное значение поля.
*/
std::string unescapeField(const std::string &field) {
  std::string result;
  bool escaping = false;
  for (char character : field) {
    if (!escaping && character == '\\') {
      escaping = true;
      continue;
    }
    if (escaping) {
      if (character == 'n') {
        result.push_back('\n');
      } else if (character == 'r') {
        result.push_back('\r');
      } else if (character == '\\' || character == '|') {
        result.push_back(character);
      } else {
        throw std::invalid_argument("Неизвестная escape-последовательность");
      }
      escaping = false;
    } else {
      result.push_back(character);
    }
  }
  if (escaping) {
    throw std::invalid_argument("Незавершенная escape-последовательность");
  }
  return result;
}

/*!
\brief Делит строку файла на поля.
\param[in] line Строка с сериализованной задачей.
\return Список восстановленных полей задачи.
*/
std::vector<std::string> splitLine(const std::string &line) {
  std::vector<std::string> fields;
  std::string current;
  bool escaping = false;
  for (char character : line) {
    if (!escaping && character == '\\') {
      escaping = true;
      current.push_back(character);
      continue;
    }
    if (!escaping && character == '|') {
      fields.push_back(unescapeField(current));
      current.clear();
      continue;
    }
    escaping = false;
    current.push_back(character);
  }
  fields.push_back(unescapeField(current));
  return fields;
}

}  // namespace

/*!
\brief Преобразует важность задачи в строку.
\param[in] importance Уровень важности задачи.
\return Строка low, medium или high.
*/
std::string importanceToString(Importance importance) {
  switch (importance) {
    case Importance::Low:
      return "low";
    case Importance::Medium:
      return "medium";
    case Importance::High:
      return "high";
  }
  throw std::invalid_argument("Неизвестная важность");
}

/*!
\brief Преобразует строку в уровень важности задачи.
\param[in] text Текстовое значение важности.
\return Значение перечисления Importance.
*/
Importance importanceFromString(const std::string &text) {
  const std::string normalized = toLower(trim(text));
  if (normalized == "low" || normalized == "низкая" || normalized == "низкий" ||
      normalized == "н") {
    return Importance::Low;
  }
  if (normalized == "medium" || normalized == "средняя" || normalized == "средний" ||
      normalized == "с") {
    return Importance::Medium;
  }
  if (normalized == "high" || normalized == "высокая" || normalized == "высокий" ||
      normalized == "в") {
    return Importance::High;
  }
  throw std::invalid_argument("Неизвестная важность: " + text);
}

/*!
\brief Преобразует статус задачи в строку.
\param[in] status Статус задачи.
\return Строка active или done.
*/
std::string statusToString(Status status) {
  switch (status) {
    case Status::Active:
      return "active";
    case Status::Done:
      return "done";
  }
  throw std::invalid_argument("Неизвестный статус");
}

/*!
\brief Преобразует строку в статус задачи.
\param[in] text Текстовое значение статуса.
\return Значение перечисления Status.
*/
Status statusFromString(const std::string &text) {
  const std::string normalized = toLower(trim(text));
  if (normalized == "active" || normalized == "активна" || normalized == "активная" ||
      normalized == "а") {
    return Status::Active;
  }
  if (normalized == "done" || normalized == "выполнена" || normalized == "выполненная" ||
      normalized == "готово" || normalized == "г") {
    return Status::Done;
  }
  throw std::invalid_argument("Неизвестный статус: " + text);
}

/*!
\brief Разбирает дату из строки формата ДД.ММ.ГГГГ.
\param[in] text Строка с датой.
\return Распознанная дата.
*/
Date parseDate(const std::string &text) {
  const std::string normalized = trim(text);
  const auto isDigit = [](char character) {
    return std::isdigit(static_cast<unsigned char>(character)) != 0;
  };

  if (normalized.size() == 10 && normalized.at(2) == '.' && normalized.at(5) == '.') {
    if (!std::all_of(normalized.begin(), normalized.begin() + 2, isDigit) ||
        !std::all_of(normalized.begin() + 3, normalized.begin() + 5, isDigit) ||
        !std::all_of(normalized.begin() + 6, normalized.end(), isDigit)) {
      throw std::invalid_argument("Дата должна содержать цифры");
    }
    Date date{std::stoi(normalized.substr(6, 4)), std::stoi(normalized.substr(3, 2)),
              std::stoi(normalized.substr(0, 2))};
    validateDate(date);
    return date;
  }

  if (normalized.size() == 10 && normalized.at(2) == ' ' && normalized.at(5) == ' ') {
    if (!std::all_of(normalized.begin(), normalized.begin() + 2, isDigit) ||
        !std::all_of(normalized.begin() + 3, normalized.begin() + 5, isDigit) ||
        !std::all_of(normalized.begin() + 6, normalized.end(), isDigit)) {
      throw std::invalid_argument("Дата должна содержать цифры");
    }
    Date date{std::stoi(normalized.substr(6, 4)), std::stoi(normalized.substr(3, 2)),
              std::stoi(normalized.substr(0, 2))};
    validateDate(date);
    return date;
  }

  // Старый формат нужен, чтобы читать уже сохраненные файлы.
  if (normalized.size() == 10 && normalized.at(4) == '-' && normalized.at(7) == '-') {
    if (!std::all_of(normalized.begin(), normalized.begin() + 4, isDigit) ||
        !std::all_of(normalized.begin() + 5, normalized.begin() + 7, isDigit) ||
        !std::all_of(normalized.begin() + 8, normalized.end(), isDigit)) {
      throw std::invalid_argument("Дата должна содержать цифры");
    }
    Date date{std::stoi(normalized.substr(0, 4)), std::stoi(normalized.substr(5, 2)),
              std::stoi(normalized.substr(8, 2))};
    validateDate(date);
    return date;
  }

  throw std::invalid_argument("Дата должна иметь формат ДД.ММ.ГГГГ");
}

/*!
\brief Форматирует дату в строку ДД.ММ.ГГГГ.
\param[in] date Дата для форматирования.
\return Строковое представление даты.
*/
std::string dateToString(const Date &date) {
  validateDate(date);
  std::ostringstream stream;
  stream << std::setw(2) << std::setfill('0') << date.day << '.' << std::setw(2)
         << std::setfill('0') << date.month << '.' << std::setw(4) << std::setfill('0')
         << date.year;
  return stream.str();
}

/*!
\brief Сравнивает две даты.
\param[in] left Первая дата.
\param[in] right Вторая дата.
\return Отрицательное число, если left раньше right; 0, если даты равны; положительное число, если
left позже right.
*/
int compareDates(const Date &left, const Date &right) {
  validateDate(left);
  validateDate(right);
  if (left.year != right.year) {
    return left.year - right.year;
  }
  if (left.month != right.month) {
    return left.month - right.month;
  }
  return left.day - right.day;
}

/*!
\brief Считает расстояние между двумя датами.
\param[in] from Начальная дата.
\param[in] to Конечная дата.
\return Количество дней от from до to.
*/
int daysBetween(const Date &from, const Date &to) {
  return static_cast<int>((toSysDays(to) - toSysDays(from)).count());
}

/*!
\brief Преобразует задачу в строку для хранения в файле.
\param[in] task Задача для сериализации.
\return Строка с полями задачи, разделенными символом '|'.
*/
std::string serializeTask(const Task &task) {
  validateTask(task);
  std::ostringstream stream;
  stream << task.id << '|' << escapeField(task.description) << '|' << dateToString(task.deadline)
         << '|' << escapeField(task.category) << '|' << importanceToString(task.importance) << '|'
         << statusToString(task.status);
  return stream.str();
}

/*!
\brief Восстанавливает задачу из строки файла.
\param[in] line Строка с сериализованной задачей.
\return Восстановленная задача.
*/
Task deserializeTask(const std::string &line) {
  const std::vector<std::string> fields = splitLine(line);
  if (fields.size() != 6) {
    throw std::invalid_argument("Строка задачи должна содержать 6 полей");
  }
  Task task;
  std::size_t position = 0;
  task.id = std::stoi(fields.at(0), &position);
  if (position != fields.at(0).size()) {
    throw std::invalid_argument("ID содержит лишние символы");
  }
  task.description = fields.at(1);
  task.deadline = parseDate(fields.at(2));
  task.category = fields.at(3);
  task.importance = importanceFromString(fields.at(4));
  task.status = statusFromString(fields.at(5));
  validateTask(task);
  return task;
}

/*!
\brief Проверяет, соответствует ли задача заданному фильтру.
\param[in] task Задача для проверки.
\param[in] filter Параметры фильтрации.
\return true, если задача подходит под фильтр, иначе false.
*/
bool taskMatchesFilter(const Task &task, const TaskFilter &filter) {
  validateTask(task);
  if (filter.keyword.has_value()) {
    const std::string query = toLower(trim(filter.keyword.value()));
    const std::string text = toLower(task.description + " " + task.category);
    if (!query.empty() && text.find(query) == std::string::npos) {
      return false;
    }
  }
  if (filter.importance.has_value() && task.importance != filter.importance.value()) {
    return false;
  }
  if (filter.status.has_value() && task.status != filter.status.value()) {
    return false;
  }
  if (filter.daysFromBaseDate.has_value()) {
    const int days = filter.daysFromBaseDate.value();
    if (days < 0) {
      throw std::invalid_argument("Количество дней не может быть отрицательным");
    }
    const int distance = daysBetween(filter.baseDate, task.deadline);
    return distance >= 0 && distance <= days;
  }
  return true;
}

/*!
\brief Сортирует задачи по приближению дедлайна.
\param[in,out] tasks Список задач для сортировки.
*/
void sortByDeadline(std::vector<Task> &tasks) {
  std::sort(tasks.begin(), tasks.end(), [](const Task &left, const Task &right) {
    const int comparison = compareDates(left.deadline, right.deadline);
    if (comparison != 0) {
      return comparison < 0;
    }
    return left.id < right.id;
  });
}

}  // namespace task_manager
