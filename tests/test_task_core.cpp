#include <stdexcept>
#include <vector>

#include "doctest.h"
#include "task.h"

using namespace task_manager;

namespace {

Task makeTask(int id, const std::string& description, const std::string& deadline,
              const std::string& category, Importance importance = Importance::Medium,
              Status status = Status::Active) {
  return Task{id, description, parseDate(deadline), category, importance, status};
}

void checkTaskEquals(const Task& task, int id, const std::string& description,
                     const std::string& deadline, const std::string& category,
                     Importance importance, Status status) {
  CHECK_EQ(task.id, id);
  CHECK_EQ(task.description, description);
  CHECK_EQ(dateToString(task.deadline), deadline);
  CHECK_EQ(task.category, category);
  CHECK_EQ(task.importance, importance);
  CHECK_EQ(task.status, status);
}

}  // namespace

TEST_CASE("Участник 1: parseDate принимает поддерживаемые форматы и отклоняет ошибки") {
  SUBCASE("Новый пользовательский формат с точками") {
    const Date date = parseDate("01.06.2026");

    CHECK_EQ(date.year, 2026);
    CHECK_EQ(date.month, 6);
    CHECK_EQ(date.day, 1);
    CHECK_EQ(dateToString(date), "01.06.2026");
  }

  SUBCASE("Формат с пробелами принимается") {
    CHECK_EQ(dateToString(parseDate("15 06 2026")), "15.06.2026");
  }

  SUBCASE("Старый файловый формат сохраняет совместимость с уже записанными задачами") {
    CHECK_EQ(dateToString(parseDate("2026-06-01")), "01.06.2026");
  }

  SUBCASE("Пробелы по краям даты игнорируются") {
    CHECK_EQ(dateToString(parseDate("  05.01.2026  ")), "05.01.2026");
  }

  SUBCASE("Проверяются граничные календарные значения") {
    CHECK_EQ(dateToString(parseDate("01.01.1900")), "01.01.1900");
    CHECK_EQ(dateToString(parseDate("31.12.3000")), "31.12.3000");
    CHECK_EQ(dateToString(parseDate("29.02.2024")), "29.02.2024");
    CHECK_EQ(dateToString(parseDate("29.02.2000")), "29.02.2000");
  }

  SUBCASE("Неверные разделители и неполные даты отклоняются") {
    CHECK_THROWS_AS(parseDate("2024/02/29"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("1.06.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("01.6.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("01.06.26"), std::invalid_argument);
  }

  SUBCASE("Символы вместо цифр отклоняются") {
    CHECK_THROWS_AS(parseDate("aa.06.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("01.bb.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("01.06.year"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("abcd-02-29"), std::invalid_argument);
  }

  SUBCASE("Невозможные календарные значения отклоняются") {
    CHECK_THROWS_AS(parseDate("30.02.2024"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("29.02.2023"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("29.02.1900"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("31.04.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("00.06.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("10.00.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("10.13.2026"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("31.12.1899"), std::invalid_argument);
    CHECK_THROWS_AS(parseDate("01.01.3001"), std::invalid_argument);
  }
}

TEST_CASE("Участник 1: dateToString форматирует корректные даты и отклоняет некорректные") {
  SUBCASE("День и месяц из одной цифры дополняются нулями") {
    CHECK_EQ(dateToString(Date{2026, 1, 5}), "05.01.2026");
    CHECK_EQ(dateToString(Date{2026, 9, 9}), "09.09.2026");
  }

  SUBCASE("Последний день месяца форматируется корректно") {
    CHECK_EQ(dateToString(Date{2026, 12, 31}), "31.12.2026");
    CHECK_EQ(dateToString(Date{2024, 2, 29}), "29.02.2024");
  }

  SUBCASE("Некорректные даты отклоняются до форматирования") {
    CHECK_THROWS_AS(dateToString(Date{2026, 13, 1}), std::invalid_argument);
    CHECK_THROWS_AS(dateToString(Date{2026, 0, 1}), std::invalid_argument);
    CHECK_THROWS_AS(dateToString(Date{2026, 2, 30}), std::invalid_argument);
    CHECK_THROWS_AS(dateToString(Date{1899, 12, 31}), std::invalid_argument);
  }
}

TEST_CASE("Участник 1: daysBetween считает расстояние между датами") {
  SUBCASE("Положительное расстояние внутри одного месяца") {
    CHECK_EQ(daysBetween(parseDate("01.06.2026"), parseDate("10.06.2026")), 9);
  }

  SUBCASE("Одинаковые даты дают ноль дней") {
    CHECK_EQ(daysBetween(parseDate("10.06.2026"), parseDate("10.06.2026")), 0);
  }

  SUBCASE("Обратный порядок дат дает отрицательное расстояние") {
    CHECK_EQ(daysBetween(parseDate("10.06.2026"), parseDate("01.06.2026")), -9);
  }

  SUBCASE("Високосный день учитывается") {
    CHECK_EQ(daysBetween(parseDate("28.02.2024"), parseDate("01.03.2024")), 2);
  }

  SUBCASE("Переход через новый год учитывается") {
    CHECK_EQ(daysBetween(parseDate("31.12.2026"), parseDate("01.01.2027")), 1);
  }

  SUBCASE("Некорректные входные даты отклоняются") {
    CHECK_THROWS_AS(daysBetween(Date{2026, 2, 30}, parseDate("01.03.2026")), std::invalid_argument);
    CHECK_THROWS_AS(daysBetween(parseDate("01.03.2026"), Date{2026, 4, 31}), std::invalid_argument);
  }
}

TEST_CASE("Участник 1: compareDates проверяет порядок дат и валидирует входные данные") {
  SUBCASE("Более ранние даты считаются меньшими") {
    CHECK_LT(compareDates(parseDate("01.06.2026"), parseDate("10.06.2026")), 0);
    CHECK_LT(compareDates(parseDate("01.05.2026"), parseDate("01.06.2026")), 0);
    CHECK_LT(compareDates(parseDate("31.12.2025"), parseDate("01.01.2026")), 0);
  }

  SUBCASE("Одинаковые даты возвращают ноль") {
    CHECK_EQ(compareDates(parseDate("10.06.2026"), parseDate("10.06.2026")), 0);
  }

  SUBCASE("Более поздние даты считаются большими") {
    CHECK_GT(compareDates(parseDate("20.06.2026"), parseDate("10.06.2026")), 0);
    CHECK_GT(compareDates(parseDate("01.07.2026"), parseDate("30.06.2026")), 0);
    CHECK_GT(compareDates(parseDate("01.01.2027"), parseDate("31.12.2026")), 0);
  }

  SUBCASE("Некорректные даты отклоняются") {
    CHECK_THROWS_AS(compareDates(Date{2026, 0, 1}, parseDate("10.06.2026")), std::invalid_argument);
    CHECK_THROWS_AS(compareDates(parseDate("10.06.2026"), Date{2026, 6, 31}),
                    std::invalid_argument);
  }
}

TEST_CASE("Участник 1: преобразование важности принимает синонимы и отклоняет ошибки") {
  SUBCASE("Значения перечисления преобразуются в строки для хранения") {
    CHECK_EQ(importanceToString(Importance::Low), "low");
    CHECK_EQ(importanceToString(Importance::Medium), "medium");
    CHECK_EQ(importanceToString(Importance::High), "high");
  }

  SUBCASE("Английские значения не зависят от регистра и пробелов по краям") {
    CHECK_EQ(importanceFromString(" low "), Importance::Low);
    CHECK_EQ(importanceFromString("MEDIUM"), Importance::Medium);
    CHECK_EQ(importanceFromString("High"), Importance::High);
  }

  SUBCASE("Русские значения и короткие команды принимаются") {
    CHECK_EQ(importanceFromString("низкая"), Importance::Low);
    CHECK_EQ(importanceFromString("низкий"), Importance::Low);
    CHECK_EQ(importanceFromString("н"), Importance::Low);
    CHECK_EQ(importanceFromString("средняя"), Importance::Medium);
    CHECK_EQ(importanceFromString("средний"), Importance::Medium);
    CHECK_EQ(importanceFromString("с"), Importance::Medium);
    CHECK_EQ(importanceFromString("высокая"), Importance::High);
    CHECK_EQ(importanceFromString("высокий"), Importance::High);
    CHECK_EQ(importanceFromString("в"), Importance::High);
  }

  SUBCASE("Неизвестный текст и некорректные значения перечисления отклоняются") {
    CHECK_THROWS_AS(importanceFromString("срочно"), std::invalid_argument);
    CHECK_THROWS_AS(importanceFromString(""), std::invalid_argument);
    CHECK_THROWS_AS(importanceToString(static_cast<Importance>(100)), std::invalid_argument);
  }
}

TEST_CASE("Участник 1: преобразование статуса принимает синонимы и отклоняет ошибки") {
  SUBCASE("Значения перечисления преобразуются в строки для хранения") {
    CHECK_EQ(statusToString(Status::Active), "active");
    CHECK_EQ(statusToString(Status::Done), "done");
  }

  SUBCASE("Английские значения не зависят от регистра и пробелов по краям") {
    CHECK_EQ(statusFromString(" active "), Status::Active);
    CHECK_EQ(statusFromString("DONE"), Status::Done);
  }

  SUBCASE("Русские значения и короткие команды принимаются") {
    CHECK_EQ(statusFromString("активна"), Status::Active);
    CHECK_EQ(statusFromString("активная"), Status::Active);
    CHECK_EQ(statusFromString("а"), Status::Active);
    CHECK_EQ(statusFromString("выполнена"), Status::Done);
    CHECK_EQ(statusFromString("выполненная"), Status::Done);
    CHECK_EQ(statusFromString("готово"), Status::Done);
    CHECK_EQ(statusFromString("г"), Status::Done);
  }

  SUBCASE("Неизвестный текст и некорректные значения перечисления отклоняются") {
    CHECK_THROWS_AS(statusFromString("ожидание"), std::invalid_argument);
    CHECK_THROWS_AS(statusFromString(""), std::invalid_argument);
    CHECK_THROWS_AS(statusToString(static_cast<Status>(100)), std::invalid_argument);
  }
}

TEST_CASE("Участник 1: serializeTask записывает корректные задачи и экранирует символы") {
  SUBCASE("Обычная активная задача сериализуется в файловый формат") {
    const Task task = makeTask(1, "Сделать проект", "15.06.2026", "учеба", Importance::High);

    CHECK_EQ(serializeTask(task), "1|Сделать проект|15.06.2026|учеба|high|active");
  }

  SUBCASE("Выполненная задача с разделителями, слешами и переносами экранируется") {
    const Task task{7,
                    "Текст | с разделителем\\и переносом\nстроки",
                    parseDate("15.06.2026"),
                    "учеба\rcpp",
                    Importance::High,
                    Status::Done};

    CHECK_EQ(serializeTask(task),
             "7|Текст \\| с разделителем\\\\и переносом\\nстроки|15.06.2026|учеба\\rcpp|high|done");
  }

  SUBCASE("Некорректные обязательные поля задачи отклоняются") {
    CHECK_THROWS_AS(serializeTask(Task{0, "Bad", parseDate("01.06.2026"), "study", Importance::Low,
                                       Status::Active}),
                    std::invalid_argument);
    CHECK_THROWS_AS(serializeTask(Task{1, "", parseDate("01.06.2026"), "study", Importance::Low,
                                       Status::Active}),
                    std::invalid_argument);
    CHECK_THROWS_AS(serializeTask(Task{1, "   ", parseDate("01.06.2026"), "study", Importance::Low,
                                       Status::Active}),
                    std::invalid_argument);
    CHECK_THROWS_AS(
        serializeTask(Task{1, "Bad", parseDate("01.06.2026"), "", Importance::Low, Status::Active}),
        std::invalid_argument);
    CHECK_THROWS_AS(serializeTask(Task{1, "Bad", parseDate("01.06.2026"), "   ", Importance::Low,
                                       Status::Active}),
                    std::invalid_argument);
    CHECK_THROWS_AS(
        serializeTask(Task{1, "Bad", Date{2026, 2, 30}, "study", Importance::Low, Status::Active}),
        std::invalid_argument);
  }
}

TEST_CASE("Участник 1: deserializeTask восстанавливает поля и отклоняет битые строки") {
  SUBCASE("Обычная строка восстанавливается в задачу") {
    const Task task = deserializeTask("2|Доклад|05.06.2026|учеба|medium|active");

    checkTaskEquals(task, 2, "Доклад", "05.06.2026", "учеба", Importance::Medium, Status::Active);
  }

  SUBCASE("Старый сохраненный формат даты восстанавливается") {
    const Task task = deserializeTask("3|Старый формат|2026-06-05|архив|low|done");

    checkTaskEquals(task, 3, "Старый формат", "05.06.2026", "архив", Importance::Low, Status::Done);
  }

  SUBCASE("Экранированные разделители, слеши и переносы восстанавливаются") {
    const Task original{7,
                        "Текст | с разделителем\\и переносом\nстроки",
                        parseDate("15.06.2026"),
                        "учеба\rcpp",
                        Importance::High,
                        Status::Done};
    const Task restored = deserializeTask(serializeTask(original));

    checkTaskEquals(restored, original.id, original.description, "15.06.2026", original.category,
                    Importance::High, Status::Done);
  }

  SUBCASE("Неверное количество полей отклоняется") {
    CHECK_THROWS_AS(deserializeTask("1|too|few"), std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1|too|many|fields|low|active|extra"), std::invalid_argument);
  }

  SUBCASE("Некорректный идентификатор отклоняется") {
    CHECK_THROWS_AS(deserializeTask("abc|Text|01.06.2026|study|low|active"), std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1abc|Text|01.06.2026|study|low|active"),
                    std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("0|Text|01.06.2026|study|low|active"), std::invalid_argument);
  }

  SUBCASE("Некорректно экранированные поля отклоняются") {
    CHECK_THROWS_AS(deserializeTask("1|Text\\q|01.06.2026|study|low|active"),
                    std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1|Text\\|01.06.2026|study|low|active"), std::invalid_argument);
  }

  SUBCASE("Некорректные значения задачи отклоняются") {
    CHECK_THROWS_AS(deserializeTask("1||01.06.2026|study|low|active"), std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1|Text|01.06.2026||low|active"), std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1|Text|30.02.2026|study|low|active"), std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1|Text|01.06.2026|study|urgent|active"),
                    std::invalid_argument);
    CHECK_THROWS_AS(deserializeTask("1|Text|01.06.2026|study|low|waiting"), std::invalid_argument);
  }
}

TEST_CASE("Участник 1: taskMatchesFilter проверяет ключевые слова, важность, статус и дни") {
  const Task activeProject =
      makeTask(3, "Write C++ Project", "05.06.2026", "study", Importance::High);
  const Task doneHome =
      makeTask(4, "Купить продукты", "08.06.2026", "дом", Importance::Low, Status::Done);

  SUBCASE("Пустой фильтр принимает корректные задачи") {
    CHECK(taskMatchesFilter(activeProject, TaskFilter{}));
    CHECK(taskMatchesFilter(doneHome, TaskFilter{}));
  }

  SUBCASE("Ключевое слово ищется в описании и категории") {
    TaskFilter filter;
    filter.keyword = "project";
    CHECK(taskMatchesFilter(activeProject, filter));

    filter.keyword = "STUDY";
    CHECK(taskMatchesFilter(activeProject, filter));

    filter.keyword = "дом";
    CHECK(taskMatchesFilter(doneHome, filter));

    filter.keyword = "биология";
    CHECK_FALSE(taskMatchesFilter(activeProject, filter));
  }

  SUBCASE("Пустое ключевое слово ничего не отфильтровывает") {
    TaskFilter filter;
    filter.keyword = "   ";

    CHECK(taskMatchesFilter(activeProject, filter));
  }

  SUBCASE("Фильтр по важности принимает только совпадающую важность") {
    TaskFilter filter;
    filter.importance = Importance::High;
    CHECK(taskMatchesFilter(activeProject, filter));
    CHECK_FALSE(taskMatchesFilter(doneHome, filter));

    filter.importance = Importance::Low;
    CHECK_FALSE(taskMatchesFilter(activeProject, filter));
    CHECK(taskMatchesFilter(doneHome, filter));
  }

  SUBCASE("Фильтр по статусу принимает только совпадающий статус") {
    TaskFilter filter;
    filter.status = Status::Active;
    CHECK(taskMatchesFilter(activeProject, filter));
    CHECK_FALSE(taskMatchesFilter(doneHome, filter));

    filter.status = Status::Done;
    CHECK_FALSE(taskMatchesFilter(activeProject, filter));
    CHECK(taskMatchesFilter(doneHome, filter));
  }

  SUBCASE("Фильтр ближайших дней включает базовую дату и верхнюю границу") {
    TaskFilter filter;
    filter.baseDate = parseDate("05.06.2026");
    filter.daysFromBaseDate = 0;
    CHECK(taskMatchesFilter(activeProject, filter));
    CHECK_FALSE(taskMatchesFilter(doneHome, filter));

    filter.daysFromBaseDate = 3;
    CHECK(taskMatchesFilter(activeProject, filter));
    CHECK(taskMatchesFilter(doneHome, filter));
  }

  SUBCASE("Фильтр ближайших дней отклоняет задачи раньше базовой даты") {
    TaskFilter filter;
    filter.baseDate = parseDate("09.06.2026");
    filter.daysFromBaseDate = 10;

    CHECK_FALSE(taskMatchesFilter(activeProject, filter));
    CHECK_FALSE(taskMatchesFilter(doneHome, filter));
  }

  SUBCASE("Комбинированный фильтр требует выполнения всех условий") {
    TaskFilter filter;
    filter.keyword = "project";
    filter.importance = Importance::High;
    filter.status = Status::Active;
    filter.baseDate = parseDate("01.06.2026");
    filter.daysFromBaseDate = 7;

    CHECK(taskMatchesFilter(activeProject, filter));

    filter.status = Status::Done;
    CHECK_FALSE(taskMatchesFilter(activeProject, filter));
  }

  SUBCASE("Некорректный фильтр и некорректная задача отклоняются") {
    TaskFilter negativeDays;
    negativeDays.daysFromBaseDate = -1;
    CHECK_THROWS_AS(taskMatchesFilter(activeProject, negativeDays), std::invalid_argument);

    TaskFilter badBaseDate;
    badBaseDate.baseDate = Date{2026, 2, 30};
    badBaseDate.daysFromBaseDate = 5;
    CHECK_THROWS_AS(taskMatchesFilter(activeProject, badBaseDate), std::invalid_argument);

    CHECK_THROWS_AS(taskMatchesFilter(Task{0, "Bad", parseDate("05.06.2026"), "study",
                                           Importance::High, Status::Active},
                                      TaskFilter{}),
                    std::invalid_argument);
  }
}

TEST_CASE("Участник 1: sortByDeadline упорядочивает задачи по дедлайну и идентификатору") {
  SUBCASE("Пустой список и список из одного элемента допускаются") {
    std::vector<Task> empty;
    CHECK_NOTHROW(sortByDeadline(empty));
    CHECK(empty.empty());

    std::vector<Task> one{makeTask(1, "Одна", "10.06.2026", "учеба")};
    CHECK_NOTHROW(sortByDeadline(one));
    CHECK_EQ(one.front().id, 1);
  }

  SUBCASE("Задачи сортируются по ближайшему дедлайну") {
    std::vector<Task> tasks{
        makeTask(3, "Позже", "20.06.2026", "учеба", Importance::Low),
        makeTask(2, "Середина", "15.06.2026", "учеба", Importance::High),
        makeTask(1, "Раньше", "10.06.2026", "учеба", Importance::Medium),
    };

    sortByDeadline(tasks);

    CHECK_EQ(tasks.at(0).id, 1);
    CHECK_EQ(tasks.at(1).id, 2);
    CHECK_EQ(tasks.at(2).id, 3);
  }

  SUBCASE("Одинаковый дедлайн дополнительно сортируется по идентификатору") {
    std::vector<Task> tasks{
        makeTask(5, "Пятое", "10.06.2026", "учеба"),
        makeTask(2, "Второе", "10.06.2026", "учеба"),
        makeTask(3, "Третье", "10.06.2026", "учеба"),
    };

    sortByDeadline(tasks);

    CHECK_EQ(tasks.at(0).id, 2);
    CHECK_EQ(tasks.at(1).id, 3);
    CHECK_EQ(tasks.at(2).id, 5);
  }

  SUBCASE("Некорректная дата в сравниваемых задачах отклоняется") {
    std::vector<Task> tasks{
        Task{2, "Ошибка", Date{2026, 2, 30}, "учеба", Importance::Low, Status::Active},
        makeTask(1, "Нормальная", "10.06.2026", "учеба"),
    };

    CHECK_THROWS_AS(sortByDeadline(tasks), std::invalid_argument);
  }
}
