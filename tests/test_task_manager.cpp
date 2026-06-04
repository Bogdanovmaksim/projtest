#include <filesystem>
#include <fstream>
#include <sstream>

#include "doctest.h"
#include "task_manager.h"

using namespace task_manager;

namespace {

std::filesystem::path tempPath(const std::string& name) {
  return name;
}

void removeFiles(const std::filesystem::path& storage, const std::filesystem::path& archive) {
  std::filesystem::remove(storage);
  std::filesystem::remove(archive);
}

}  // namespace

/*!
\brief Проверка добавления первой задачи.
\details Проверяется, что первая корректно добавленная
задача получает идентификатор id = 1 и статус Active.
*/
TEST_CASE("addTask adds first task with id 1 and active status") {
  const auto storage = tempPath("manager_add_tasks_1.txt");
  const auto archive = tempPath("manager_add_archive_1.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task task =
      manager.addTask("Доклад",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::High);

  REQUIRE(task.id == 1);
  REQUIRE(task.status == Status::Active);

  removeFiles(storage, archive);
}

/*!
\brief Проверка последовательного увеличения идентификаторов.
\details Проверяется, что при добавлении нескольких задач
каждая новая задача получает следующий уникальный id.
*/
TEST_CASE("addTask assigns sequential ids") {
  const auto storage = tempPath("manager_add_tasks_2.txt");
  const auto archive = tempPath("manager_add_archive_2.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task first =
      manager.addTask("Доклад",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::High);

  const Task second =
      manager.addTask("Магазин",
                      parseDate("11.06.2026"),
                      "дом",
                      Importance::Low);

  const Task third =
      manager.addTask("Спорт",
                      parseDate("12.06.2026"),
                      "здоровье",
                      Importance::Medium);

  REQUIRE(first.id == 1);
  REQUIRE(second.id == 2);
  REQUIRE(third.id == 3);

  removeFiles(storage, archive);
}

/*!
\brief Проверка изменения количества задач после добавления.
\details После успешного добавления задачи размер
коллекции задач должен увеличиться.
*/
TEST_CASE("addTask increases task count") {
  const auto storage = tempPath("manager_add_tasks_3.txt");
  const auto archive = tempPath("manager_add_archive_3.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  REQUIRE(manager.tasks().empty());

  manager.addTask("Задача",
                  parseDate("10.06.2026"),
                  "тест",
                  Importance::Low);

  REQUIRE(manager.tasks().size() == 1);

  removeFiles(storage, archive);
}

/*!
\brief Проверка обработки пустого описания.
\details Попытка создать задачу с пустым описанием
должна приводить к выбросу исключения.
*/
TEST_CASE("addTask rejects empty description") {
  const auto storage = tempPath("manager_add_tasks_4.txt");
  const auto archive = tempPath("manager_add_archive_4.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  REQUIRE_THROWS_AS(
      manager.addTask("",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::High),
      std::invalid_argument);

  REQUIRE(manager.tasks().empty());

  removeFiles(storage, archive);
}


/*!
\brief Проверка обработки пустой категории.
\details Попытка создать задачу без категории
должна завершаться ошибкой.
*/
TEST_CASE("addTask rejects empty category") {
  const auto storage = tempPath("manager_add_tasks_5.txt");
  const auto archive = tempPath("manager_add_archive_5.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  REQUIRE_THROWS_AS(
      manager.addTask("Доклад",
                      parseDate("10.06.2026"),
                      "",
                      Importance::High),
      std::invalid_argument);

  REQUIRE(manager.tasks().empty());

  removeFiles(storage, archive);
}

/*!
\brief Проверка некорректной календарной даты.
\details Попытка создать задачу с невозможной
датой должна завершаться ошибкой.
*/
TEST_CASE("addTask rejects invalid calendar date") {
  const auto storage = tempPath("manager_add_tasks_6.txt");
  const auto archive = tempPath("manager_add_archive_6.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  REQUIRE_THROWS_AS(
      manager.addTask("Ошибка",
                      Date{2026, 2, 30},
                      "учеба",
                      Importance::High),
      std::invalid_argument);

  REQUIRE(manager.tasks().empty());

  removeFiles(storage, archive);
}

/*!
\brief Проверка дедлайна в прошлом.
\details Система не должна разрешать добавление
задач с прошедшей датой выполнения.
*/
TEST_CASE("addTask rejects past deadline") {
  const auto storage = tempPath("manager_add_tasks_7.txt");
  const auto archive = tempPath("manager_add_archive_7.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  REQUIRE_THROWS_AS(
      manager.addTask("Прошлая задача",
                      parseDate("12.03.2026"),
                      "учеба",
                      Importance::Low),
      std::invalid_argument);

  REQUIRE(manager.tasks().empty());

  removeFiles(storage, archive);
}

/*!
\brief Проверка изменения только описания задачи.
\details Проверяется, что при изменении описания
остальные поля остаются без изменений.
*/
TEST_CASE("updateTask changes only description") {
  const auto storage = tempPath("manager_update_tasks_1.txt");
  const auto archive = tempPath("manager_update_archive_1.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task created =
      manager.addTask("Старый текст",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::Low);

  TaskUpdate update;
  update.description = "Новый текст";

  const Task changed = manager.updateTask(created.id, update);

  REQUIRE(changed.description == "Новый текст");
  REQUIRE(dateToString(changed.deadline) == "10.06.2026");
  REQUIRE(changed.category == "учеба");
  REQUIRE(changed.importance == Importance::Low);
  REQUIRE(changed.status == Status::Active);

  removeFiles(storage, archive);
}

/*!
\brief Проверка изменения нескольких полей задачи.
\details Проверяется одновременное изменение
дедлайна, категории и важности.
*/
TEST_CASE("updateTask changes several fields") {
  const auto storage = tempPath("manager_update_tasks_2.txt");
  const auto archive = tempPath("manager_update_archive_2.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task created =
      manager.addTask("Задача",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::Low);

  TaskUpdate update;
  update.deadline = parseDate("15.06.2026");
  update.category = "работа";
  update.importance = Importance::High;

  const Task changed = manager.updateTask(created.id, update);

  REQUIRE(dateToString(changed.deadline) == "15.06.2026");
  REQUIRE(changed.category == "работа");
  REQUIRE(changed.importance == Importance::High);
  REQUIRE(changed.description == "Задача");
  REQUIRE(changed.status == Status::Active);

  removeFiles(storage, archive);
}

/*!
\brief Проверка сохранения id и количества задач.
\details Обновление должно изменять существующую
задачу, а не создавать новую.
*/
TEST_CASE("updateTask keeps id and task count unchanged") {
  const auto storage = tempPath("manager_update_tasks_3.txt");
  const auto archive = tempPath("manager_update_archive_3.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task created =
      manager.addTask("Задача",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::Low);

  const size_t oldSize = manager.tasks().size();

  TaskUpdate update;
  update.description = "Обновлено";

  const Task changed = manager.updateTask(created.id, update);

  REQUIRE(changed.id == created.id);
  REQUIRE(manager.tasks().size() == oldSize);

  removeFiles(storage, archive);
}

/*!
\brief Проверка пустого набора изменений.
\details Пустой TaskUpdate не должен изменять
существующую задачу.
*/
TEST_CASE("updateTask with empty update keeps task unchanged") {
  const auto storage = tempPath("manager_update_tasks_4.txt");
  const auto archive = tempPath("manager_update_archive_4.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task created =
      manager.addTask("Исходная задача",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::Low);

  TaskUpdate update;

  const Task changed = manager.updateTask(created.id, update);

  REQUIRE(changed.id == created.id);
  REQUIRE(changed.description == created.description);
  REQUIRE(changed.category == created.category);
  REQUIRE(changed.importance == created.importance);
  REQUIRE(changed.status == created.status);
  REQUIRE(dateToString(changed.deadline) ==
          dateToString(created.deadline));

  removeFiles(storage, archive);
}

/*!
\brief Проверка ошибок валидации.
\details Проверяется, что пустые строки,
невалидные даты и некорректные значения
не допускаются при обновлении задачи.
*/
TEST_CASE("updateTask rejects invalid update values") {
  const auto storage = tempPath("manager_update_tasks_5.txt");
  const auto archive = tempPath("manager_update_archive_5.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task created =
      manager.addTask("Исходная задача",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::Low);

  TaskUpdate emptyDescription;
  emptyDescription.description = "";

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, emptyDescription),
      std::invalid_argument);

  TaskUpdate spacesDescription;
  spacesDescription.description = "   ";

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, spacesDescription),
      std::invalid_argument);

  TaskUpdate emptyCategory;
  emptyCategory.category = "";

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, emptyCategory),
      std::invalid_argument);

  TaskUpdate spacesCategory;
  spacesCategory.category = "   ";

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, spacesCategory),
      std::invalid_argument);

  TaskUpdate invalidDate;
  invalidDate.deadline = Date{2026, 2, 30};

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, invalidDate),
      std::invalid_argument);

  TaskUpdate pastDeadline;
  pastDeadline.deadline = parseDate("12.03.2026");

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, pastDeadline),
      std::invalid_argument);

  removeFiles(storage, archive);
}

/*!
\brief Проверка работоспособности после ошибки.
\details После неудачного обновления задача
должна продолжать корректно обновляться.
*/
TEST_CASE("updateTask allows valid update after failed update") {
  const auto storage = tempPath("manager_update_tasks_6.txt");
  const auto archive = tempPath("manager_update_archive_6.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  const Task created =
      manager.addTask("Старая задача",
                      parseDate("10.06.2026"),
                      "учеба",
                      Importance::Low);

  TaskUpdate invalidUpdate;
  invalidUpdate.description = "";

  REQUIRE_THROWS_AS(
      manager.updateTask(created.id, invalidUpdate),
      std::invalid_argument);

  TaskUpdate validUpdate;
  validUpdate.description = "Новая задача";
  validUpdate.category = "работа";

  const Task updated =
      manager.updateTask(created.id, validUpdate);

  REQUIRE(updated.description == "Новая задача");
  REQUIRE(updated.category == "работа");
  REQUIRE(updated.id == created.id);

  removeFiles(storage, archive);
}

/*!
\brief Проверка фильтрации выполненных задач.
\details Проверяется, что фильтр по статусу Done
возвращает только выполненные задачи.
*/
TEST_CASE("filterTasks returns done tasks") {
  const auto storage = tempPath("manager_filter_tasks_1.txt");
  const auto archive = tempPath("manager_filter_archive_1.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  manager.addTask("Активная",
                  parseDate("10.06.2026"),
                  "учеба",
                  Importance::Low);

  const Task done =
      manager.addTask("Выполненная",
                      parseDate("11.06.2026"),
                      "дом",
                      Importance::High);

  TaskUpdate update;
  update.status = Status::Done;
  manager.updateTask(done.id, update);

  TaskFilter filter;
  filter.status = Status::Done;

  const auto result = manager.filterTasks(filter);

  REQUIRE(result.size() == 1);
  REQUIRE(result.front().description == "Выполненная");
  REQUIRE(result.front().status == Status::Done);

  removeFiles(storage, archive);
}

/*!
\brief Проверка фильтрации по ближайшим дням.
\details Проверяется, что возвращаются только задачи,
дедлайн которых попадает в указанный диапазон дней.
*/
TEST_CASE("filterTasks filters by nearest days") {
  const auto storage = tempPath("manager_filter_tasks_2.txt");
  const auto archive = tempPath("manager_filter_archive_2.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  manager.addTask("Близкая",
                  parseDate("12.06.2026"),
                  "учеба",
                  Importance::Low);

  manager.addTask("Далекая",
                  parseDate("25.06.2026"),
                  "дом",
                  Importance::Low);

  TaskFilter filter;
  filter.baseDate = parseDate("10.06.2026");
  filter.daysFromBaseDate = 5;

  const auto result = manager.filterTasks(filter);

  REQUIRE(result.size() == 1);
  REQUIRE(result.front().description == "Близкая");

  removeFiles(storage, archive);
}

/*!
\brief Проверка комбинированной фильтрации.
\details Проверяется, что несколько условий фильтра
применяются одновременно.
*/
TEST_CASE("filterTasks applies several filters together") {
  const auto storage = tempPath("manager_filter_tasks_3.txt");
  const auto archive = tempPath("manager_filter_archive_3.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  manager.addTask("Сделать доклад",
                  parseDate("12.06.2026"),
                  "учеба",
                  Importance::High);

  manager.addTask("Сделать конспект",
                  parseDate("12.06.2026"),
                  "учеба",
                  Importance::Low);

  manager.addTask("Купить продукты",
                  parseDate("12.06.2026"),
                  "дом",
                  Importance::High);

  TaskFilter filter;
  filter.keyword = "доклад";
  filter.importance = Importance::High;
  filter.status = Status::Active;
  filter.baseDate = parseDate("10.06.2026");
  filter.daysFromBaseDate = 5;

  const auto result = manager.filterTasks(filter);

  REQUIRE(result.size() == 1);
  REQUIRE(result.front().description == "Сделать доклад");
  REQUIRE(result.front().importance == Importance::High);
  REQUIRE(result.front().status == Status::Active);

  removeFiles(storage, archive);
}

/*!
\brief Проверка пустого результата фильтрации.
\details Если ни одна задача не подходит под условия,
фильтр должен вернуть пустой список.
*/
TEST_CASE("filterTasks returns empty result when nothing matches") {
  const auto storage = tempPath("manager_filter_tasks_4.txt");
  const auto archive = tempPath("manager_filter_archive_4.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  manager.addTask("Доклад",
                  parseDate("10.06.2026"),
                  "учеба",
                  Importance::High);

  TaskFilter filter;
  filter.keyword = "спорт";

  const auto result = manager.filterTasks(filter);

  REQUIRE(result.empty());

  removeFiles(storage, archive);
}

/*!
\brief Проверка пустого фильтра.
\details Если фильтр не содержит условий,
должны вернуться все текущие задачи.
*/
TEST_CASE("filterTasks with empty filter returns all tasks") {
  const auto storage = tempPath("manager_filter_tasks_5.txt");
  const auto archive = tempPath("manager_filter_archive_5.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  manager.addTask("Первая",
                  parseDate("10.06.2026"),
                  "учеба",
                  Importance::Low);

  manager.addTask("Вторая",
                  parseDate("11.06.2026"),
                  "дом",
                  Importance::High);

  TaskFilter filter;

  const auto result = manager.filterTasks(filter);

  REQUIRE(result.size() == 2);
  REQUIRE(result.size() == manager.tasks().size());

  removeFiles(storage, archive);
}

/*!
\brief Проверка некорректного диапазона дней.
\details Отрицательное количество дней для фильтрации
должно приводить к исключению.
*/
TEST_CASE("filterTasks rejects negative day range") {
  const auto storage = tempPath("manager_filter_tasks_7.txt");
  const auto archive = tempPath("manager_filter_archive_7.txt");

  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  manager.addTask("Задача",
                  parseDate("10.06.2026"),
                  "учеба",
                  Importance::Low);

  TaskFilter filter;
  filter.baseDate = parseDate("10.06.2026");
  filter.daysFromBaseDate = -5;

  REQUIRE_THROWS_AS(
      manager.filterTasks(filter),
      std::invalid_argument);

  removeFiles(storage, archive);
}