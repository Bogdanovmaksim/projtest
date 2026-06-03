#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "task.h"

namespace task_manager {

/*!
\brief Набор полей для изменения задачи.
*/
struct TaskUpdate {
    std::optional<std::string> description;  ///< Новое описание.
    std::optional<Date> deadline;            ///< Новый дедлайн.
    std::optional<std::string> category;     ///< Новая категория.
    std::optional<Importance> importance;    ///< Новая важность.
    std::optional<Status> status;            ///< Новый статус.
};

/*!
\brief Менеджер задач с загрузкой и сохранением в файлы.
*/
class TaskManager {
   public:
    /*!
    \brief Создает менеджер задач.
    \param[in] storagePath Файл задач.
    \param[in] archivePath Файл архива.
    */
    TaskManager(std::filesystem::path storagePath, std::filesystem::path archivePath);

    /*!
    \brief Загружает задачи из файла.
    */
    void load();

    /*!
    \brief Сохраняет задачи в файл.
    */
    void save() const;

    /*!
    \brief Добавляет новую задачу.
    \param[in] description Описание.
    \param[in] deadline Дедлайн.
    \param[in] category Категория.
    \param[in] importance Важность.
    \return Созданная задача.
    \throws std::invalid_argument Если дедлайн находится в прошлом.
    */
    Task addTask(std::string description, Date deadline, std::string category,
                 Importance importance);

    /*!
    \brief Изменяет существующую задачу.
    \param[in] id Идентификатор.
    \param[in] update Новые значения полей.
    \return Обновленная задача.
    \throws std::invalid_argument Если новый дедлайн находится в прошлом.
    \throws std::invalid_argument Если новый дедлайн раньше текущего дедлайна задачи.
    */
    Task updateTask(int id, const TaskUpdate& update);

    /*!
    \brief Отмечает задачу выполненной.
    \param[in] id Идентификатор.
    */
    void markDone(int id);

    /*!
    \brief Удаляет задачу.
    \param[in] id Идентификатор.
    \return Удаленная задача.
    */
    Task removeTask(int id);

    /*!
    \brief Возвращает задачи, подходящие под фильтр.
    \param[in] filter Фильтр.
    \return Список задач.
    */
    std::vector<Task> filterTasks(const TaskFilter& filter) const;

    /*!
    \brief Возвращает задачи, отсортированные по дедлайну.
    \return Отсортированный список.
    */
    std::vector<Task> sortedByDeadline() const;

    /*!
    \brief Архивирует выполненные задачи.
    \return Количество архивированных задач.
    */
    std::size_t archiveCompleted();

    /*!
    \brief Возвращает текущий список задач.
    \return Константная ссылка на задачи.
    */
    const std::vector<Task>& tasks() const;

   private:
    std::filesystem::path storagePath_;  ///< Путь к основному файлу задач.
    std::filesystem::path archivePath_;  ///< Путь к файлу архива выполненных задач.
    std::vector<Task> tasks_;            ///< Текущий список задач.
    int nextId_{1};                      ///< Следующий свободный идентификатор задачи.

    /*!
    \brief Находит задачу по идентификатору.
    \param[in] id Идентификатор задачи.
    \return Изменяемая ссылка на найденную задачу.
    */
    Task& findTask(int id);

    /*!
    \brief Находит задачу по идентификатору без возможности изменения.
    \param[in] id Идентификатор задачи.
    \return Константная ссылка на найденную задачу.
    */
    const Task& findTask(int id) const;

    /*!
    \brief Пересчитывает следующий свободный идентификатор.
    */
    void refreshNextId();
};

}  // namespace task_manager
