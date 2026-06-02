#include "doctest.h"
#include "task.h"

using namespace task_manager;

TEST_CASE("participant 1 parseDate checks correct and incorrect dates") {
    const Date date = parseDate("2026-06-01");
    CHECK_EQ(date.year, 2026);
    CHECK_EQ(date.month, 6);
    CHECK_EQ(date.day, 1);
    CHECK_EQ(dateToString(parseDate("2024-02-29")), "2024-02-29");
    CHECK_THROWS_AS(parseDate("2024-02-30"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("2024/02/29"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("abcd-02-29"), std::invalid_argument);
}

TEST_CASE("participant 1 daysBetween and dateToString cover several scenarios") {
    CHECK_EQ(dateToString(Date{2026, 1, 5}), "2026-01-05");
    CHECK_EQ(daysBetween(parseDate("2026-06-01"), parseDate("2026-06-10")), 9);
    CHECK_EQ(daysBetween(parseDate("2024-02-28"), parseDate("2024-03-01")), 2);
    CHECK_EQ(daysBetween(parseDate("2026-06-10"), parseDate("2026-06-01")), -9);
    CHECK_THROWS_AS(dateToString(Date{2026, 13, 1}), std::invalid_argument);
}

TEST_CASE("participant 1 converts importance and status from Russian text") {
    CHECK_EQ(importanceFromString("низкая"), Importance::Low);
    CHECK_EQ(importanceFromString("средняя"), Importance::Medium);
    CHECK_EQ(importanceFromString("высокая"), Importance::High);
    CHECK_EQ(importanceFromString("в"), Importance::High);
    CHECK_EQ(statusFromString("активна"), Status::Active);
    CHECK_EQ(statusFromString("выполнена"), Status::Done);
    CHECK_THROWS_AS(importanceFromString("срочно"), std::invalid_argument);
    CHECK_THROWS_AS(statusFromString("ожидание"), std::invalid_argument);
}

TEST_CASE("participant 1 serialize and deserialize preserve task fields") {
    Task task{7, "Текст | с разделителем", parseDate("2026-06-15"), "учеба\\cpp",
              Importance::High, Status::Active};
    const Task restored = deserializeTask(serializeTask(task));
    CHECK_EQ(restored.id, 7);
    CHECK_EQ(restored.description, task.description);
    CHECK_EQ(restored.category, task.category);
    CHECK_EQ(restored.importance, Importance::High);
    CHECK_EQ(restored.status, Status::Active);
    CHECK_THROWS_AS(deserializeTask("1|too|few"), std::invalid_argument);
    CHECK_THROWS_AS(serializeTask(Task{0, "Bad", parseDate("2026-06-01"), "study",
                                       Importance::Low, Status::Active}),
                    std::invalid_argument);
}

TEST_CASE("participant 1 taskMatchesFilter checks keyword importance status and days") {
    const Task task{3, "Сделать проект C++", parseDate("2026-06-05"), "учеба",
                    Importance::High, Status::Active};
    TaskFilter filter;
    CHECK(taskMatchesFilter(task, filter));
    filter.keyword = "проект";
    CHECK(taskMatchesFilter(task, filter));
    filter.keyword = "биология";
    CHECK(!taskMatchesFilter(task, filter));
    filter.keyword = std::nullopt;
    filter.importance = Importance::Low;
    CHECK(!taskMatchesFilter(task, filter));
    filter.importance = Importance::High;
    filter.status = Status::Active;
    CHECK(taskMatchesFilter(task, filter));
    filter.baseDate = parseDate("2026-06-01");
    filter.daysFromBaseDate = 3;
    CHECK(!taskMatchesFilter(task, filter));
    filter.daysFromBaseDate = 7;
    CHECK(taskMatchesFilter(task, filter));
}

TEST_CASE("participant 1 sortByDeadline sorts by date and id") {
    std::vector<Task> tasks{
        Task{3, "Позже", parseDate("2026-06-20"), "учеба", Importance::Low, Status::Active},
        Task{2, "Раньше 2", parseDate("2026-06-10"), "учеба", Importance::High, Status::Active},
        Task{1, "Раньше 1", parseDate("2026-06-10"), "учеба", Importance::Medium,
             Status::Active},
    };
    sortByDeadline(tasks);
    CHECK_EQ(tasks.at(0).id, 1);
    CHECK_EQ(tasks.at(1).id, 2);
    CHECK_EQ(tasks.at(2).id, 3);
    tasks.push_back(Task{4, "Ошибка", Date{2026, 2, 30}, "учеба", Importance::Low,
                         Status::Active});
    CHECK_THROWS_AS(sortByDeadline(tasks), std::invalid_argument);
}
