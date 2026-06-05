#pragma once

#include "../migration.h"

namespace db
{
namespace migrations
{

/**
 * @brief Миграция v1: Создание начальной структуры базы данных.
 */
class V1_InitialSchema final : public IMigration
{
public:
    unsigned int version() const override
    {
        return 1;
    }

    std::string description() const override
    {
        return "Создаёт первоначальную схему базы данных.";
    }

    void up(std::shared_ptr<IConnection> connection) override
    {
        // Пользователи и команды
        connection->execute(
            "CREATE TABLE User ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    login TEXT NOT NULL UNIQUE,"
            "    firstName TEXT,"
            "    middleName TEXT,"
            "    lastName TEXT,"
            "    email TEXT NOT NULL UNIQUE,"
            "    passwordHash TEXT NOT NULL,"
            "    needChangePassword INTEGER NOT NULL DEFAULT 1,"
            "    isBlocked INTEGER NOT NULL DEFAULT 0,"
            "    isSuperAdmin INTEGER NOT NULL DEFAULT 0,"
            "    isHidden INTEGER NOT NULL DEFAULT 0"
            ")"
        );

        connection->execute(
            "CREATE TABLE Team ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    caption TEXT NOT NULL,"
            "    description TEXT"
            ")"
        );

        connection->execute(
            "CREATE TABLE Role ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    caption TEXT NOT NULL,"
            "    description TEXT"
            ")"
        );

        connection->execute(
            "CREATE TABLE UserTeamRole ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    userId INTEGER NOT NULL,"
            "    teamId INTEGER NOT NULL,"
            "    roleId INTEGER NOT NULL,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE,"
            "    UNIQUE(userId, teamId, roleId)"
            ")"
        );

        connection->execute(
            "CREATE TABLE Rule ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    roleId INTEGER NOT NULL,"
            "    isRootProjectCreator INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE RuleProject ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    ruleId INTEGER NOT NULL,"
            "    projectId INTEGER NOT NULL,"
            "    isReader INTEGER NOT NULL DEFAULT 0,"
            "    isWriter INTEGER NOT NULL DEFAULT 0,"
            "    isProjectEditor INTEGER NOT NULL DEFAULT 0,"
            "    isPhaseEditor INTEGER NOT NULL DEFAULT 0,"
            "    isBoardEditor INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,"
            "    UNIQUE(ruleId, projectId)"
            ")"
        );

        connection->execute(
            "CREATE TABLE RuleItemType ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    ruleId INTEGER NOT NULL,"
            "    itemTypeId INTEGER NOT NULL,"
            "    isReader INTEGER NOT NULL DEFAULT 0,"
            "    isWriter INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,"
            "    UNIQUE(ruleId, itemTypeId)"
            ")"
        );

        connection->execute(
            "CREATE TABLE RuleState ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    ruleId INTEGER NOT NULL,"
            "    stateId INTEGER NOT NULL,"
            "    isStateAllowed INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,"
            "    UNIQUE(ruleId, stateId)"
            ")"
        );

        connection->execute(
            "CREATE TABLE RoleMenuItem ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    roleId INTEGER NOT NULL,"
            "    caption TEXT NOT NULL,"
            "    link TEXT,"
            "    icon TEXT,"
            "    FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE"
            ")"
        );

        // Проекты и фазы
        connection->execute(
            "CREATE TABLE Project ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    parentId INTEGER,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    searchCaption TEXT,"
            "    searchDescription TEXT,"
            "    createdAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    updatedAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    isArchive INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (parentId) REFERENCES Project(id) ON DELETE SET NULL"
            ")"
        );

        connection->execute(
            "CREATE TABLE Phase ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    projectId INTEGER NOT NULL,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    beginDate INTEGER,"
            "    endDate INTEGER,"
            "    isArchive INTEGER NOT NULL DEFAULT 0,"
            "    searchCaption TEXT,"
            "    searchDescription TEXT,"
            "    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE ProjectTeam ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    projectId INTEGER NOT NULL,"
            "    teamId INTEGER NOT NULL,"
            "    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE,"
            "    UNIQUE(projectId, teamId)"
            ")"
        );

        // Типы элементов и поля
        connection->execute(
            "CREATE TABLE Workflow ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    caption TEXT NOT NULL,"
            "    description TEXT"
            ")"
        );

        connection->execute(
            "CREATE TABLE State ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    workflowId INTEGER NOT NULL,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    weight INTEGER DEFAULT 0,"
            "    orderNumber INTEGER DEFAULT 0,"
            "    isArchive INTEGER NOT NULL DEFAULT 0,"
            "    isQueue INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE Edge ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    beginStateId INTEGER NOT NULL,"
            "    endStateId INTEGER NOT NULL,"
            "    FOREIGN KEY (beginStateId) REFERENCES State(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (endStateId) REFERENCES State(id) ON DELETE CASCADE,"
            "    UNIQUE(beginStateId, endStateId)"
            ")"
        );

        connection->execute(
            "CREATE TABLE ItemType ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    workflowId INTEGER NOT NULL,"
            "    defaultStateId INTEGER,"
            "    caption TEXT NOT NULL,"
            "    kind TEXT NOT NULL CHECK (kind IN ('issue', 'folder', 'test-case', 'test-cycle', 'test-execution', 'requirement')),"
            "    defaultContent TEXT,"
            "    FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE FieldType ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemTypeId INTEGER NOT NULL,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    valueType TEXT NOT NULL CHECK (valueType IN ("
            "        'String', 'MarkdownText', 'Integer', 'Float', 'Bool', "
            "        'DateTime', 'Date', 'Uri', 'Select', 'SelectMulti',"
            "        'ItemId', 'UserId', 'ProjectId', 'PhaseId',"
            "        'ItemsIds', 'UsersIds', 'ProjectsIds', 'PhasesIds'"
            "    )),"
            "    isBoardVisible INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE FieldTypePossibleValue ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    fieldTypeId INTEGER NOT NULL,"
            "    value TEXT NOT NULL,"
            "    FOREIGN KEY (fieldTypeId) REFERENCES FieldType(id) ON DELETE CASCADE"
            ")"
        );

        // Элементы
        connection->execute(
            "CREATE TABLE Item ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemTypeId INTEGER NOT NULL,"
            "    parentId INTEGER,"
            "    stateId INTEGER NOT NULL,"
            "    phaseId INTEGER,"
            "    caption TEXT NOT NULL,"
            "    content TEXT,"
            "    searchCaption TEXT,"
            "    searchContent TEXT,"
            "    isDeleted INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (parentId) REFERENCES Item(id) ON DELETE SET NULL,"
            "    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL"
            ")"
        );

        connection->execute(
            "CREATE TABLE ItemField ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemId INTEGER NOT NULL,"
            "    fieldTypeId INTEGER NOT NULL,"
            "    value TEXT,"
            "    searchValue TEXT,"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (fieldTypeId) REFERENCES FieldType(id) ON DELETE CASCADE,"
            "    UNIQUE(itemId, fieldTypeId)"
            ")"
        );

        connection->execute(
            "CREATE TABLE ItemHistory ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemId INTEGER NOT NULL,"
            "    userId INTEGER NOT NULL,"
            "    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    diff TEXT,"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE ItemUserState ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemId INTEGER NOT NULL,"
            "    userId INTEGER NOT NULL,"
            "    stateId INTEGER NOT NULL,"
            "    comment TEXT,"
            "    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE"
            ")"
        );

        // Связи между элементами
        connection->execute(
            "CREATE TABLE LinkType ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    sourceItemTypeId INTEGER NOT NULL,"
            "    destinationItemTypeId INTEGER NOT NULL,"
            "    isBidirectional INTEGER NOT NULL DEFAULT 0,"
            "    caption TEXT NOT NULL,"
            "    FOREIGN KEY (sourceItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (destinationItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE ItemLink ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    linkTypeId INTEGER NOT NULL,"
            "    sourceItemId INTEGER NOT NULL,"
            "    destinationItemId INTEGER NOT NULL,"
            "    FOREIGN KEY (linkTypeId) REFERENCES LinkType(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (sourceItemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (destinationItemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    UNIQUE(linkTypeId, sourceItemId, destinationItemId)"
            ")"
        );

        // Документы
        connection->execute(
            "CREATE TABLE Document ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    path TEXT NOT NULL,"
            "    filename TEXT NOT NULL,"
            "    size INTEGER NOT NULL,"
            "    mimeType TEXT,"
            "    uploadedAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    uploadedByUserId INTEGER NOT NULL,"
            "    searchCaption TEXT,"
            "    searchDescription TEXT,"
            "    FOREIGN KEY (uploadedByUserId) REFERENCES User(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE ItemDocument ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemId INTEGER NOT NULL,"
            "    documentId INTEGER NOT NULL,"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (documentId) REFERENCES Document(id) ON DELETE CASCADE,"
            "    UNIQUE(itemId, documentId)"
            ")"
        );

        // Комментарии
        connection->execute(
            "CREATE TABLE Comment ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    userId INTEGER NOT NULL,"
            "    itemId INTEGER NOT NULL,"
            "    createdAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    content TEXT NOT NULL,"
            "    searchContent TEXT,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE CommentDocument ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    commentId INTEGER NOT NULL,"
            "    documentId INTEGER NOT NULL,"
            "    FOREIGN KEY (commentId) REFERENCES Comment(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (documentId) REFERENCES Document(id) ON DELETE CASCADE,"
            "    UNIQUE(commentId, documentId)"
            ")"
        );

        // Доски
        connection->execute(
            "CREATE TABLE Board ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    workflowId INTEGER NOT NULL,"
            "    projectId INTEGER NOT NULL,"
            "    phaseId INTEGER,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL"
            ")"
        );

        connection->execute(
            "CREATE TABLE BoardColumn ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    boardId INTEGER NOT NULL,"
            "    stateId INTEGER NOT NULL,"
            "    orderNumber INTEGER DEFAULT 0,"
            "    settings TEXT,"
            "    FOREIGN KEY (boardId) REFERENCES Board(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,"
            "    UNIQUE(boardId, stateId)"
            ")"
        );

        // Рабочее время
        connection->execute(
            "CREATE TABLE StandardDay ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    weekDayNumber INTEGER NOT NULL CHECK (weekDayNumber BETWEEN 0 AND 6),"
            "    weekOrder INTEGER DEFAULT 0,"
            "    isWorkDay INTEGER NOT NULL DEFAULT 1,"
            "    beginWorkTime TEXT,"
            "    endWorkTime TEXT,"
            "    breakDuration INTEGER DEFAULT 0"
            ")"
        );

        connection->execute(
            "CREATE TABLE SpecialDay ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    date INTEGER NOT NULL UNIQUE,"
            "    isWorkDay INTEGER NOT NULL,"
            "    beginWorkTime TEXT,"
            "    endWorkTime TEXT,"
            "    breakDuration INTEGER DEFAULT 0"
            ")"
        );

        connection->execute(
            "CREATE TABLE UserDay ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    userId INTEGER NOT NULL,"
            "    date INTEGER NOT NULL,"
            "    isWorkDay INTEGER NOT NULL,"
            "    beginWorkTime TEXT,"
            "    endWorkTime TEXT,"
            "    breakDuration INTEGER DEFAULT 0,"
            "    description TEXT,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,"
            "    UNIQUE(userId, date)"
            ")"
        );

        // Сообщения и уведомления
        connection->execute(
            "CREATE TABLE PrivateMessage ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    senderUserId INTEGER NOT NULL,"
            "    receiverUserId INTEGER NOT NULL,"
            "    creationTimestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    content TEXT NOT NULL,"
            "    isViewed INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (senderUserId) REFERENCES User(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (receiverUserId) REFERENCES User(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE TeamMessage ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    senderUserId INTEGER NOT NULL,"
            "    teamId INTEGER NOT NULL,"
            "    creationTimestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    content TEXT NOT NULL,"
            "    FOREIGN KEY (senderUserId) REFERENCES User(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE UserNotification ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    userId INTEGER NOT NULL,"
            "    itemId INTEGER NOT NULL,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    UNIQUE(userId, itemId)"
            ")"
        );

        // Действия и задачи
        connection->execute(
            "CREATE TABLE UserAction ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    userId INTEGER NOT NULL,"
            "    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE UserTodo ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    userId INTEGER NOT NULL,"
            "    isDone INTEGER NOT NULL DEFAULT 0,"
            "    caption TEXT NOT NULL,"
            "    searchCaption TEXT,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE"
            ")"
        );

        connection->execute(
            "CREATE TABLE Plan ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    phaseId INTEGER NOT NULL,"
            "    basePlanId INTEGER,"
            "    caption TEXT NOT NULL,"
            "    description TEXT,"
            "    isActive INTEGER NOT NULL DEFAULT 0,"
            "    createdAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
            "    createdByUserId INTEGER NOT NULL,"
            "    activatedAt INTEGER,"
            "    activatedByUserId INTEGER,"
            "    FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (basePlanId) REFERENCES Plan(id) ON DELETE SET NULL,"
            "    FOREIGN KEY (createdByUserId) REFERENCES User(id),"
            "    FOREIGN KEY (activatedByUserId) REFERENCES User(id)"
            ")"
        );

        connection->execute(
            "CREATE TABLE PlanItem ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    itemId INTEGER NOT NULL,"
            "    userId INTEGER,"
            "    planId INTEGER NOT NULL,"
            "    startDate INTEGER NOT NULL,"
            "    endDate INTEGER NOT NULL,"
            "    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,"
            "    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE SET NULL,"
            "    FOREIGN KEY (planId) REFERENCES Plan(id) ON DELETE CASCADE,"
            "    UNIQUE(planId, itemId)"
            ")"
        );

        // Индексы
        connection->execute("CREATE INDEX idx_item_itemTypeId ON Item(itemTypeId)");
        connection->execute("CREATE INDEX idx_item_parentId ON Item(parentId)");
        connection->execute("CREATE INDEX idx_item_stateId ON Item(stateId)");
        connection->execute("CREATE INDEX idx_item_phaseId ON Item(phaseId)");
        connection->execute("CREATE INDEX idx_item_isDeleted ON Item(isDeleted)");
        connection->execute("CREATE INDEX idx_itemField_itemId ON ItemField(itemId)");
        connection->execute("CREATE INDEX idx_itemField_fieldTypeId ON ItemField(fieldTypeId)");
        connection->execute("CREATE INDEX idx_itemField_searchValue ON ItemField(searchValue)");
        connection->execute("CREATE INDEX idx_comment_itemId ON Comment(itemId)");
        connection->execute("CREATE INDEX idx_comment_userId ON Comment(userId)");
        connection->execute("CREATE INDEX idx_itemHistory_itemId ON ItemHistory(itemId)");
        connection->execute("CREATE INDEX idx_itemHistory_userId ON ItemHistory(userId)");
        connection->execute("CREATE INDEX idx_itemHistory_timestamp ON ItemHistory(timestamp)");
        connection->execute("CREATE INDEX idx_itemUserState_itemId ON ItemUserState(itemId)");
        connection->execute("CREATE INDEX idx_itemUserState_userId ON ItemUserState(userId)");
        connection->execute("CREATE INDEX idx_userTeamRole_userId ON UserTeamRole(userId)");
        connection->execute("CREATE INDEX idx_userTeamRole_teamId ON UserTeamRole(teamId)");
        connection->execute("CREATE INDEX idx_userTeamRole_roleId ON UserTeamRole(roleId)");
        connection->execute("CREATE INDEX idx_ruleProject_ruleId ON RuleProject(ruleId)");
        connection->execute("CREATE INDEX idx_ruleProject_projectId ON RuleProject(projectId)");
        connection->execute("CREATE INDEX idx_ruleItemType_ruleId ON RuleItemType(ruleId)");
        connection->execute("CREATE INDEX idx_ruleItemType_itemTypeId ON RuleItemType(itemTypeId)");
        connection->execute("CREATE INDEX idx_ruleState_ruleId ON RuleState(ruleId)");
        connection->execute("CREATE INDEX idx_ruleState_stateId ON RuleState(stateId)");
        connection->execute("CREATE INDEX idx_plan_phaseId ON Plan(phaseId)");
        connection->execute("CREATE INDEX idx_plan_basePlanId ON Plan(basePlanId)");
        connection->execute("CREATE INDEX idx_plan_isActive ON Plan(isActive)");
        connection->execute("CREATE INDEX idx_plan_createdByUserId ON Plan(createdByUserId)");
        connection->execute("CREATE INDEX idx_plan_activatedByUserId ON Plan(activatedByUserId)");
        connection->execute("CREATE INDEX idx_planItem_planId ON PlanItem(planId)");
        connection->execute("CREATE INDEX idx_planItem_itemId ON PlanItem(itemId)");
        connection->execute("CREATE INDEX idx_planItem_userId ON PlanItem(userId)");
        connection->execute("CREATE INDEX idx_planItem_dates ON PlanItem(startDate, endDate)");
    }

    void down(std::shared_ptr<IConnection> connection) override
    {
        // Удаляем индексы
        connection->execute("DROP INDEX IF EXISTS idx_planItem_dates");
        connection->execute("DROP INDEX IF EXISTS idx_planItem_userId");
        connection->execute("DROP INDEX IF EXISTS idx_planItem_itemId");
        connection->execute("DROP INDEX IF EXISTS idx_planItem_planId");
        connection->execute("DROP INDEX IF EXISTS idx_plan_activatedByUserId");
        connection->execute("DROP INDEX IF EXISTS idx_plan_createdByUserId");
        connection->execute("DROP INDEX IF EXISTS idx_plan_isActive");
        connection->execute("DROP INDEX IF EXISTS idx_plan_basePlanId");
        connection->execute("DROP INDEX IF EXISTS idx_plan_phaseId");
        connection->execute("DROP INDEX IF EXISTS idx_ruleState_stateId");
        connection->execute("DROP INDEX IF EXISTS idx_ruleState_ruleId");
        connection->execute("DROP INDEX IF EXISTS idx_ruleItemType_itemTypeId");
        connection->execute("DROP INDEX IF EXISTS idx_ruleItemType_ruleId");
        connection->execute("DROP INDEX IF EXISTS idx_ruleProject_projectId");
        connection->execute("DROP INDEX IF EXISTS idx_ruleProject_ruleId");
        connection->execute("DROP INDEX IF EXISTS idx_userTeamRole_roleId");
        connection->execute("DROP INDEX IF EXISTS idx_userTeamRole_teamId");
        connection->execute("DROP INDEX IF EXISTS idx_userTeamRole_userId");
        connection->execute("DROP INDEX IF EXISTS idx_itemUserState_userId");
        connection->execute("DROP INDEX IF EXISTS idx_itemUserState_itemId");
        connection->execute("DROP INDEX IF EXISTS idx_itemHistory_timestamp");
        connection->execute("DROP INDEX IF EXISTS idx_itemHistory_userId");
        connection->execute("DROP INDEX IF EXISTS idx_itemHistory_itemId");
        connection->execute("DROP INDEX IF EXISTS idx_comment_userId");
        connection->execute("DROP INDEX IF EXISTS idx_comment_itemId");
        connection->execute("DROP INDEX IF EXISTS idx_itemField_searchValue");
        connection->execute("DROP INDEX IF EXISTS idx_itemField_fieldTypeId");
        connection->execute("DROP INDEX IF EXISTS idx_itemField_itemId");
        connection->execute("DROP INDEX IF EXISTS idx_item_isDeleted");
        connection->execute("DROP INDEX IF EXISTS idx_item_phaseId");
        connection->execute("DROP INDEX IF EXISTS idx_item_stateId");
        connection->execute("DROP INDEX IF EXISTS idx_item_parentId");
        connection->execute("DROP INDEX IF EXISTS idx_item_itemTypeId");

        // Удаляем таблицы в обратном порядке (с учётом внешних ключей)
        connection->execute("DROP TABLE IF EXISTS PlanItem");
        connection->execute("DROP TABLE IF EXISTS Plan");
        connection->execute("DROP TABLE IF EXISTS UserTodo");
        connection->execute("DROP TABLE IF EXISTS UserAction");
        connection->execute("DROP TABLE IF EXISTS UserNotification");
        connection->execute("DROP TABLE IF EXISTS TeamMessage");
        connection->execute("DROP TABLE IF EXISTS PrivateMessage");
        connection->execute("DROP TABLE IF EXISTS UserDay");
        connection->execute("DROP TABLE IF EXISTS SpecialDay");
        connection->execute("DROP TABLE IF EXISTS StandardDay");
        connection->execute("DROP TABLE IF EXISTS BoardColumn");
        connection->execute("DROP TABLE IF EXISTS Board");
        connection->execute("DROP TABLE IF EXISTS CommentDocument");
        connection->execute("DROP TABLE IF EXISTS Comment");
        connection->execute("DROP TABLE IF EXISTS ItemDocument");
        connection->execute("DROP TABLE IF EXISTS Document");
        connection->execute("DROP TABLE IF EXISTS ItemLink");
        connection->execute("DROP TABLE IF EXISTS LinkType");
        connection->execute("DROP TABLE IF EXISTS ItemUserState");
        connection->execute("DROP TABLE IF EXISTS ItemHistory");
        connection->execute("DROP TABLE IF EXISTS ItemField");
        connection->execute("DROP TABLE IF EXISTS Item");
        connection->execute("DROP TABLE IF EXISTS FieldTypePossibleValue");
        connection->execute("DROP TABLE IF EXISTS FieldType");
        connection->execute("DROP TABLE IF EXISTS ItemType");
        connection->execute("DROP TABLE IF EXISTS Edge");
        connection->execute("DROP TABLE IF EXISTS State");
        connection->execute("DROP TABLE IF EXISTS Workflow");
        connection->execute("DROP TABLE IF EXISTS ProjectTeam");
        connection->execute("DROP TABLE IF EXISTS Phase");
        connection->execute("DROP TABLE IF EXISTS Project");
        connection->execute("DROP TABLE IF EXISTS RoleMenuItem");
        connection->execute("DROP TABLE IF EXISTS RuleState");
        connection->execute("DROP TABLE IF EXISTS RuleItemType");
        connection->execute("DROP TABLE IF EXISTS RuleProject");
        connection->execute("DROP TABLE IF EXISTS Rule");
        connection->execute("DROP TABLE IF EXISTS UserTeamRole");
        connection->execute("DROP TABLE IF EXISTS Role");
        connection->execute("DROP TABLE IF EXISTS Team");
        connection->execute("DROP TABLE IF EXISTS User");
    }
};

} // namespace migrations
} // namespace db
