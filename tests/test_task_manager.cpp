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

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

}  // namespace

TEST_CASE("TaskManager::addTask - добавление задачи и проверка ошибок") {
  const auto storage = tempPath("manager_add_tasks.txt");
  const auto archive = tempPath("manager_add_archive.txt");
  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  // Сценарий 1: первая задача получает id 1 и остается активной.
  const Task first = manager.addTask("Доклад", parseDate("10.06.2099"), "учеба", Importance::High);
  CHECK_EQ(first.id, 1);
  CHECK_EQ(first.status, Status::Active);
  CHECK_EQ(first.description, "Доклад");
  CHECK_EQ(dateToString(first.deadline), "10.06.2099");
  CHECK_EQ(first.category, "учеба");
  CHECK_EQ(first.importance, Importance::High);

  // Сценарий 2: следующая задача получает следующий id.
  const Task second = manager.addTask("Магазин", parseDate("11.06.2099"), "дом", Importance::Low);
  CHECK_EQ(second.id, 2);
  CHECK_EQ(second.status, Status::Active);

  // Сценарий 3: третья задача добавляется в общий список, данные не теряются.
  const Task third =
      manager.addTask("Тренировка", parseDate("12.06.2099"), "спорт", Importance::Medium);
  CHECK_EQ(third.id, 3);
  CHECK_EQ(manager.tasks().size(), 3U);
  CHECK_EQ(manager.tasks().at(0).description, "Доклад");
  CHECK_EQ(manager.tasks().at(1).category, "дом");
  CHECK_EQ(manager.tasks().at(2).importance, Importance::Medium);

  // Сценарий 4: пустое описание запрещено.
  CHECK_THROWS_AS(manager.addTask("", parseDate("13.06.2099"), "учеба", Importance::High),
                  std::invalid_argument);

  // Сценарий 5: описание из одних пробелов тоже запрещено.
  CHECK_THROWS_AS(manager.addTask("   ", parseDate("13.06.2099"), "учеба", Importance::High),
                  std::invalid_argument);

  // Сценарий 6: пустая категория запрещена.
  CHECK_THROWS_AS(manager.addTask("Ошибка", parseDate("13.06.2099"), "", Importance::High),
                  std::invalid_argument);

  // Сценарий 7: невозможная календарная дата запрещена.
  CHECK_THROWS_AS(manager.addTask("Ошибка", Date{2099, 2, 30}, "учеба", Importance::High),
                  std::invalid_argument);

  // Сценарий 8: дедлайн раньше сегодняшней даты запрещен.
  CHECK_THROWS_AS(manager.addTask("Прошлая задача", parseDate("12.03.2026"), "учеба",
                                  Importance::Low),
                  std::invalid_argument);

  // Сценарий 9: после всех ошибок в списке остались только корректные задачи.
  CHECK_EQ(manager.tasks().size(), 3U);

  removeFiles(storage, archive);
}

TEST_CASE("TaskManager::updateTask - изменение задачи и проверка ошибок") {
  const auto storage = tempPath("manager_update_tasks.txt");
  const auto archive = tempPath("manager_update_archive.txt");
  removeFiles(storage, archive);

  TaskManager manager(storage, archive);
  const Task created =
      manager.addTask("Старый текст", parseDate("10.06.2099"), "учеба", Importance::Low);

  // Сценарий 1: можно изменить только описание, остальные поля сохраняются.
  TaskUpdate descriptionOnly;
  descriptionOnly.description = "Новый текст";
  const Task changedDescription = manager.updateTask(created.id, descriptionOnly);
  CHECK_EQ(changedDescription.description, "Новый текст");
  CHECK_EQ(dateToString(changedDescription.deadline), "10.06.2099");
  CHECK_EQ(changedDescription.category, "учеба");
  CHECK_EQ(changedDescription.importance, Importance::Low);

  // Сценарий 2: можно изменить сразу дедлайн, категорию и важность.
  TaskUpdate severalFields;
  severalFields.deadline = parseDate("15.06.2099");
  severalFields.category = "работа";
  severalFields.importance = Importance::High;
  const Task changedSeveral = manager.updateTask(created.id, severalFields);
  CHECK_EQ(dateToString(changedSeveral.deadline), "15.06.2099");
  CHECK_EQ(changedSeveral.category, "работа");
  CHECK_EQ(changedSeveral.importance, Importance::High);

  // Сценарий 3: можно изменить только статус задачи.
  TaskUpdate statusOnly;
  statusOnly.status = Status::Done;
  const Task changedStatus = manager.updateTask(created.id, statusOnly);
  CHECK_EQ(changedStatus.status, Status::Done);

  // Сценарий 4: пустой набор изменений не ломает текущие данные.
  TaskUpdate emptyUpdate;
  const Task unchanged = manager.updateTask(created.id, emptyUpdate);
  CHECK_EQ(unchanged.description, "Новый текст");
  CHECK_EQ(unchanged.category, "работа");
  CHECK_EQ(unchanged.status, Status::Done);

  // Сценарий 5: новый дедлайн нельзя сделать раньше текущего дедлайна.
  TaskUpdate earlierDeadline;
  earlierDeadline.deadline = parseDate("14.06.2099");
  CHECK_THROWS_AS(manager.updateTask(created.id, earlierDeadline), std::invalid_argument);

  // Сценарий 6: новый дедлайн нельзя сделать раньше сегодняшней даты.
  TaskUpdate pastDeadline;
  pastDeadline.deadline = parseDate("12.03.2026");
  CHECK_THROWS_AS(manager.updateTask(created.id, pastDeadline), std::invalid_argument);

  // Сценарий 7: пустое описание при изменении запрещено.
  TaskUpdate emptyDescription;
  emptyDescription.description = "";
  CHECK_THROWS_AS(manager.updateTask(created.id, emptyDescription), std::invalid_argument);

  // Сценарий 8: изменение несуществующей задачи вызывает исключение.
  CHECK_THROWS_AS(manager.updateTask(999, severalFields), std::runtime_error);

  // Сценарий 9: после ошибок задача сохранила последнее корректное состояние.
  CHECK_EQ(manager.tasks().front().description, "Новый текст");
  CHECK_EQ(dateToString(manager.tasks().front().deadline), "15.06.2099");
  CHECK_EQ(manager.tasks().front().category, "работа");
  CHECK_EQ(manager.tasks().front().importance, Importance::High);
  CHECK_EQ(manager.tasks().front().status, Status::Done);

  removeFiles(storage, archive);
}

TEST_CASE("TaskManager::filterTasks - фильтрация по разным условиям") {
  const auto storage = tempPath("manager_filter_tasks.txt");
  const auto archive = tempPath("manager_filter_archive.txt");
  removeFiles(storage, archive);

  TaskManager manager(storage, archive);
  const Task homework =
      manager.addTask("Поздняя домашка", parseDate("20.06.2099"), "учеба", Importance::Low);
  const Task urgent =
      manager.addTask("Срочный проект", parseDate("05.06.2099"), "работа", Importance::High);
  const Task medium =
      manager.addTask("Средний проект", parseDate("10.06.2099"), "работа", Importance::Medium);
  manager.markDone(medium.id);

  // Сценарий 1: пустой фильтр возвращает все задачи.
  TaskFilter emptyFilter;
  CHECK_EQ(manager.filterTasks(emptyFilter).size(), 3U);

  // Сценарий 2: фильтр по ключевому слову находит несколько задач.
  TaskFilter keywordFilter;
  keywordFilter.keyword = "проект";
  CHECK_EQ(manager.filterTasks(keywordFilter).size(), 2U);

  // Сценарий 3: фильтр по ключевому слову находит одну конкретную задачу.
  keywordFilter.keyword = "домашка";
  CHECK_EQ(manager.filterTasks(keywordFilter).front().id, homework.id);

  // Сценарий 4: если ключевого слова нет, список пустой.
  keywordFilter.keyword = "нет такого слова";
  CHECK_EQ(manager.filterTasks(keywordFilter).size(), 0U);

  // Сценарий 5: фильтр по высокой важности возвращает срочную задачу.
  TaskFilter importanceFilter;
  importanceFilter.importance = Importance::High;
  CHECK_EQ(manager.filterTasks(importanceFilter).front().id, urgent.id);

  // Сценарий 6: фильтр по низкой важности возвращает домашку.
  importanceFilter.importance = Importance::Low;
  CHECK_EQ(manager.filterTasks(importanceFilter).front().id, homework.id);

  // Сценарий 7: фильтр по выполненному статусу возвращает выполненную задачу.
  TaskFilter statusFilter;
  statusFilter.status = Status::Done;
  CHECK_EQ(manager.filterTasks(statusFilter).front().id, medium.id);

  // Сценарий 8: фильтр по активному статусу возвращает две невыполненные задачи.
  statusFilter.status = Status::Active;
  CHECK_EQ(manager.filterTasks(statusFilter).size(), 2U);

  // Сценарий 9: фильтр ближайших дней выбирает задачи от базовой даты до указанного срока.
  TaskFilter daysFilter;
  daysFilter.baseDate = parseDate("05.06.2099");
  daysFilter.daysFromBaseDate = 5;
  CHECK_EQ(manager.filterTasks(daysFilter).size(), 2U);

  // Сценарий 10: ноль дней означает только задачи на саму базовую дату.
  daysFilter.daysFromBaseDate = 0;
  CHECK_EQ(manager.filterTasks(daysFilter).front().id, urgent.id);

  // Сценарий 11: отрицательное количество дней считается ошибкой.
  daysFilter.daysFromBaseDate = -1;
  CHECK_THROWS_AS(manager.filterTasks(daysFilter), std::invalid_argument);

  // Сценарий 12: комбинированный фильтр применяет все условия одновременно.
  TaskFilter combinedFilter;
  combinedFilter.keyword = "проект";
  combinedFilter.importance = Importance::Medium;
  combinedFilter.status = Status::Done;
  CHECK_EQ(manager.filterTasks(combinedFilter).front().id, medium.id);

  removeFiles(storage, archive);
}

TEST_CASE("TaskManager::archiveCompleted - перенос выполненных задач в архив") {
  const auto storage = tempPath("manager_archive_tasks.txt");
  const auto archive = tempPath("manager_archive_archive.txt");
  removeFiles(storage, archive);

  TaskManager manager(storage, archive);

  // Сценарий 1: пустой список архивируется без ошибок и возвращает 0.
  CHECK_EQ(manager.archiveCompleted(), 0U);
  CHECK_EQ(manager.tasks().size(), 0U);

  const Task active =
      manager.addTask("Активная", parseDate("10.06.2099"), "учеба", Importance::Low);
  const Task doneOne =
      manager.addTask("Готовая первая", parseDate("11.06.2099"), "работа", Importance::High);
  const Task doneTwo =
      manager.addTask("Готовая вторая", parseDate("12.06.2099"), "дом", Importance::Medium);

  manager.markDone(doneOne.id);
  manager.markDone(doneTwo.id);

  // Сценарий 2: из смешанного списка архивируются только выполненные задачи.
  CHECK_EQ(manager.archiveCompleted(), 2U);

  // Сценарий 3: после архивации активная задача остается в основном списке.
  CHECK_EQ(manager.tasks().size(), 1U);
  CHECK_EQ(manager.tasks().front().id, active.id);
  CHECK_EQ(manager.tasks().front().status, Status::Active);

  // Сценарий 4: в архивном файле есть выполненные задачи.
  const std::string archiveText = readFile(archive);
  CHECK(archiveText.find("Готовая первая") != std::string::npos);
  CHECK(archiveText.find("Готовая вторая") != std::string::npos);

  // Сценарий 5: активная задача не попадает в архив.
  CHECK(archiveText.find("Активная") == std::string::npos);

  // Сценарий 6: повторная архивация без новых выполненных задач возвращает 0.
  CHECK_EQ(manager.archiveCompleted(), 0U);
  CHECK_EQ(manager.tasks().size(), 1U);

  // Сценарий 7: если оставшуюся задачу выполнить, она тоже переносится в архив.
  manager.markDone(active.id);
  CHECK_EQ(manager.archiveCompleted(), 1U);

  // Сценарий 8: после архивации всех выполненных задач основной список пуст.
  CHECK_EQ(manager.tasks().size(), 0U);

  removeFiles(storage, archive);
}
