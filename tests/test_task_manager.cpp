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

} // namespace

TEST_CASE("participant 2 addTask assigns ids and validates fields") {
    const auto storage = tempPath("manager_add_tasks.txt");
    const auto archive = tempPath("manager_add_archive.txt");
    removeFiles(storage, archive);
    TaskManager manager(storage, archive);
    const Task first = manager.addTask("Доклад", parseDate("2099-06-10"), "учеба", Importance::High);
    const Task second = manager.addTask("Магазин", parseDate("2099-06-11"), "дом", Importance::Low);
    CHECK_EQ(first.id, 1);
    CHECK_EQ(second.id, 2);
    CHECK_EQ(manager.tasks().size(), 2U);
    CHECK_THROWS_AS(manager.addTask("", parseDate("2099-06-10"), "учеба", Importance::High),
                    std::invalid_argument);
    CHECK_THROWS_AS(manager.addTask("Ошибка", Date{2026, 2, 30}, "учеба", Importance::High),
                    std::invalid_argument);
    CHECK_THROWS_AS(manager.addTask("Прошлая задача", parseDate("12.03.2026"), "учеба",
                                    Importance::Low),
                    std::invalid_argument);
    removeFiles(storage, archive);
}

TEST_CASE("participant 2 updateTask changes selected fields") {
    const auto storage = tempPath("manager_update_tasks.txt");
    const auto archive = tempPath("manager_update_archive.txt");
    removeFiles(storage, archive);
    TaskManager manager(storage, archive);
    const Task created = manager.addTask("Старый текст", parseDate("2099-06-10"), "учеба",
                                         Importance::Low);
    TaskUpdate update;
    update.description = "Новый текст";
    update.deadline = parseDate("15.06.2099");
    update.importance = Importance::High;
    const Task changed = manager.updateTask(created.id, update);
    CHECK_EQ(changed.description, "Новый текст");
    CHECK_EQ(dateToString(changed.deadline), "15.06.2099");
    CHECK_EQ(changed.category, "учеба");

    TaskUpdate earlierDeadline;
    earlierDeadline.deadline = parseDate("09.06.2099");
    CHECK_THROWS_AS(manager.updateTask(created.id, earlierDeadline), std::invalid_argument);

    TaskUpdate pastDeadline;
    pastDeadline.deadline = parseDate("12.03.2026");
    CHECK_THROWS_AS(manager.updateTask(created.id, pastDeadline), std::invalid_argument);

    CHECK_THROWS_AS(manager.updateTask(999, update), std::runtime_error);
    removeFiles(storage, archive);
}

TEST_CASE("participant 2 markDone and removeTask handle ids") {
    const auto storage = tempPath("manager_remove_tasks.txt");
    const auto archive = tempPath("manager_remove_archive.txt");
    removeFiles(storage, archive);
    TaskManager manager(storage, archive);
    const Task first = manager.addTask("Первая", parseDate("2099-06-10"), "учеба", Importance::Low);
    const Task second =
        manager.addTask("Вторая", parseDate("2099-06-11"), "учеба", Importance::Medium);
    manager.markDone(first.id);
    CHECK_EQ(manager.tasks().at(0).status, Status::Done);
    const Task removed = manager.removeTask(second.id);
    CHECK_EQ(removed.description, "Вторая");
    CHECK_EQ(manager.tasks().size(), 1U);
    CHECK_THROWS_AS(manager.removeTask(999), std::runtime_error);
    removeFiles(storage, archive);
}

TEST_CASE("participant 2 filterTasks and sortedByDeadline return correct lists") {
    const auto storage = tempPath("manager_filter_tasks.txt");
    const auto archive = tempPath("manager_filter_archive.txt");
    removeFiles(storage, archive);
    TaskManager manager(storage, archive);
    manager.addTask("Поздняя домашка", parseDate("2099-06-20"), "учеба", Importance::Low);
    manager.addTask("Срочный проект", parseDate("2099-06-05"), "работа", Importance::High);
    manager.addTask("Средний проект", parseDate("2099-06-10"), "работа", Importance::Medium);
    TaskFilter filter;
    filter.keyword = "проект";
    CHECK_EQ(manager.filterTasks(filter).size(), 2U);
    filter.importance = Importance::High;
    CHECK_EQ(manager.filterTasks(filter).front().description, "Срочный проект");
    const auto sorted = manager.sortedByDeadline();
    CHECK_EQ(sorted.at(0).description, "Срочный проект");
    CHECK_EQ(sorted.at(2).description, "Поздняя домашка");
    removeFiles(storage, archive);
}

TEST_CASE("participant 2 save and load persist tasks") {
    const auto storage = tempPath("manager_save_tasks.txt");
    const auto archive = tempPath("manager_save_archive.txt");
    removeFiles(storage, archive);
    {
        TaskManager manager(storage, archive);
        manager.addTask("Первая", parseDate("2099-06-10"), "учеба", Importance::Low);
        manager.addTask("Вторая", parseDate("2099-06-11"), "работа", Importance::High);
        manager.save();
    }
    TaskManager loaded(storage, archive);
    loaded.load();
    CHECK_EQ(loaded.tasks().size(), 2U);
    CHECK_EQ(loaded.addTask("Третья", parseDate("2099-06-12"), "дом", Importance::Medium).id, 3);
    removeFiles(storage, archive);
}

TEST_CASE("participant 2 archiveCompleted moves only completed tasks") {
    const auto storage = tempPath("manager_archive_tasks.txt");
    const auto archive = tempPath("manager_archive_archive.txt");
    removeFiles(storage, archive);
    TaskManager manager(storage, archive);
    const Task active = manager.addTask("Активная", parseDate("2099-06-10"), "учеба", Importance::Low);
    const Task done = manager.addTask("Готовая", parseDate("2099-06-11"), "работа", Importance::High);
    manager.markDone(done.id);
    CHECK_EQ(manager.archiveCompleted(), 1U);
    CHECK_EQ(manager.tasks().front().id, active.id);
    CHECK(readFile(archive).find("Готовая") != std::string::npos);
    CHECK_EQ(manager.archiveCompleted(), 0U);
    removeFiles(storage, archive);
}
