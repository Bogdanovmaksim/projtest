#include "task_manager.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace task_manager {

TaskManager::TaskManager(std::filesystem::path storagePath, std::filesystem::path archivePath)
    : storagePath_(std::move(storagePath)), archivePath_(std::move(archivePath)) {
    if (storagePath_.empty() || archivePath_.empty()) {
        throw std::invalid_argument("Пути к файлам не должны быть пустыми");
    }
}

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

void TaskManager::save() const {
    std::ofstream output(storagePath_, std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Не удалось открыть файл задач для записи");
    }
    for (const Task& task : tasks_) {
        output << serializeTask(task) << '\n';
    }
}

Task TaskManager::addTask(std::string description, Date deadline, std::string category,
                          Importance importance) {
    Task task{nextId_++, std::move(description), deadline, std::move(category), importance,
              Status::Active};
    serializeTask(task);
    tasks_.push_back(task);
    return task;
}

Task TaskManager::updateTask(int id, const TaskUpdate& update) {
    Task& task = findTask(id);
    Task candidate = task;
    if (update.description.has_value()) {
        candidate.description = update.description.value();
    }
    if (update.deadline.has_value()) {
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

void TaskManager::markDone(int id) {
    TaskUpdate update;
    update.status = Status::Done;
    updateTask(id, update);
}

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

std::vector<Task> TaskManager::filterTasks(const TaskFilter& filter) const {
    std::vector<Task> result;
    for (const Task& task : tasks_) {
        if (taskMatchesFilter(task, filter)) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<Task> TaskManager::sortedByDeadline() const {
    std::vector<Task> result = tasks_;
    sortByDeadline(result);
    return result;
}

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

const std::vector<Task>& TaskManager::tasks() const {
    return tasks_;
}

Task& TaskManager::findTask(int id) {
    const auto iterator = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& task) {
        return task.id == id;
    });
    if (iterator == tasks_.end()) {
        throw std::runtime_error("Задача не найдена");
    }
    return *iterator;
}

const Task& TaskManager::findTask(int id) const {
    const auto iterator = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& task) {
        return task.id == id;
    });
    if (iterator == tasks_.end()) {
        throw std::runtime_error("Задача не найдена");
    }
    return *iterator;
}

void TaskManager::refreshNextId() {
    nextId_ = 1;
    for (const Task& task : tasks_) {
        if (task.id >= nextId_) {
            nextId_ = task.id + 1;
        }
    }
}

} // namespace task_manager
