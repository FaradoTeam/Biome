-- =====================================================
-- SQLite скрипт создания базы данных
-- =====================================================

-- 1. Пользователи и команды
-- =====================================================

-- Пользователи
CREATE TABLE User (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    login TEXT NOT NULL UNIQUE,
    firstName TEXT,
    middleName TEXT,
    lastName TEXT,
    email TEXT NOT NULL UNIQUE,
    passwordHash TEXT NOT NULL,
    needChangePassword INTEGER NOT NULL DEFAULT 1, -- 0=false, 1=true
    isBlocked INTEGER NOT NULL DEFAULT 0,
    isSuperAdmin INTEGER NOT NULL DEFAULT 0,
    isHidden INTEGER NOT NULL DEFAULT 0
);

-- Команды
CREATE TABLE Team (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    caption TEXT NOT NULL,
    description TEXT
);

-- Роли (глобальные роли, комбинации правил)
CREATE TABLE Role (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    caption TEXT NOT NULL,
    description TEXT
);

-- Связь пользователь-команда-роль (пользователь имеет роль в команде)
CREATE TABLE UserTeamRole (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    teamId INTEGER NOT NULL,
    roleId INTEGER NOT NULL,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE,
    FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE,
    UNIQUE(userId, teamId, roleId)
);

-- Правила
CREATE TABLE Rule (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    roleId INTEGER NOT NULL,
    isRootProjectCreator INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE
);

-- Права на проекты
CREATE TABLE RuleProject (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ruleId INTEGER NOT NULL,
    projectId INTEGER NOT NULL,
    isReader INTEGER NOT NULL DEFAULT 0,
    isWriter INTEGER NOT NULL DEFAULT 0,
    isProjectEditor INTEGER NOT NULL DEFAULT 0,
    isPhaseEditor INTEGER NOT NULL DEFAULT 0,
    isBoardEditor INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,
    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,
    UNIQUE(ruleId, projectId)
);

-- Права на типы элементов
CREATE TABLE RuleItemType (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ruleId INTEGER NOT NULL,
    itemTypeId INTEGER NOT NULL,
    isReader INTEGER NOT NULL DEFAULT 0,
    isWriter INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,
    FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
    UNIQUE(ruleId, itemTypeId)
);

-- Права на состояния
CREATE TABLE RuleState (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ruleId INTEGER NOT NULL,
    stateId INTEGER NOT NULL,
    isStateAllowed INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,
    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
    UNIQUE(ruleId, stateId)
);

-- Пункты меню для ролей
CREATE TABLE RoleMenuItem (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    roleId INTEGER NOT NULL,
    caption TEXT NOT NULL,
    link TEXT,
    icon TEXT,
    FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE
);


-- 2. Проекты и фазы
-- =====================================================

-- Проекты
CREATE TABLE Project (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    parentId INTEGER,
    caption TEXT NOT NULL,
    description TEXT,
    searchCaption TEXT,
    searchDescription TEXT,
    createdAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updatedAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    isArchive INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (parentId) REFERENCES Project(id) ON DELETE SET NULL
);

-- Фазы проектов
CREATE TABLE Phase (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    projectId INTEGER NOT NULL,
    caption TEXT NOT NULL,
    description TEXT,
    beginDate INTEGER,
    endDate INTEGER,
    isArchive INTEGER NOT NULL DEFAULT 0,
    searchCaption TEXT,
    searchDescription TEXT,
    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE
);

-- Связь проект-команда
CREATE TABLE ProjectTeam (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    projectId INTEGER NOT NULL,
    teamId INTEGER NOT NULL,
    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,
    FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE,
    UNIQUE(projectId, teamId)
);


-- 3. Типы элементов и поля
-- =====================================================

-- Типы элементов
CREATE TABLE ItemType (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflowId INTEGER NOT NULL,
    defaultStateId INTEGER,
    caption TEXT NOT NULL,
    kind TEXT NOT NULL CHECK (kind IN ('issue', 'folder', 'test-case', 'test-cycle', 'test-execution', 'requirement')),
    defaultContent TEXT,
    FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE
);

-- Типы полей
CREATE TABLE FieldType (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemTypeId INTEGER NOT NULL,
    caption TEXT NOT NULL,
    description TEXT,
    valueType TEXT NOT NULL CHECK (valueType IN (
        'String', 'MarkdownText', 'Integer', 'Float', 'Bool', 
        'DateTime', 'Date', 'Uri', 'Select', 'SelectMulti',
        'ItemId', 'UserId', 'ProjectId', 'PhaseId',
        'ItemsIds', 'UsersIds', 'ProjectsIds', 'PhasesIds'
    )),
    isBoardVisible INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE
);

-- Возможные значения для полей типа Select/SelectMulti
CREATE TABLE FieldTypePossibleValue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fieldTypeId INTEGER NOT NULL,
    value TEXT NOT NULL,
    FOREIGN KEY (fieldTypeId) REFERENCES FieldType(id) ON DELETE CASCADE
);


-- 4. Рабочие процессы
-- =====================================================

-- Рабочие процессы
CREATE TABLE Workflow (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    caption TEXT NOT NULL,
    description TEXT
);

-- Состояния
CREATE TABLE State (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflowId INTEGER NOT NULL,
    caption TEXT NOT NULL,
    description TEXT,
    weight INTEGER DEFAULT 0,
    orderNumber INTEGER DEFAULT 0,
    isArchive INTEGER NOT NULL DEFAULT 0,
    isQueue INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE
);

-- Переходы между состояниями
CREATE TABLE Edge (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    beginStateId INTEGER NOT NULL,
    endStateId INTEGER NOT NULL,
    FOREIGN KEY (beginStateId) REFERENCES State(id) ON DELETE CASCADE,
    FOREIGN KEY (endStateId) REFERENCES State(id) ON DELETE CASCADE,
    UNIQUE(beginStateId, endStateId)
);


-- 5. Элементы (Item)
-- =====================================================

-- Элементы
CREATE TABLE Item (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemTypeId INTEGER NOT NULL,
    parentId INTEGER,
    stateId INTEGER NOT NULL,
    phaseId INTEGER,
    caption TEXT NOT NULL,
    content TEXT,
    searchCaption TEXT,
    searchContent TEXT,
    isDeleted INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
    FOREIGN KEY (parentId) REFERENCES Item(id) ON DELETE SET NULL,
    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
    FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
);

-- Значения полей элементов
CREATE TABLE ItemField (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemId INTEGER NOT NULL,
    fieldTypeId INTEGER NOT NULL,
    value TEXT,
    searchValue TEXT,
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
    FOREIGN KEY (fieldTypeId) REFERENCES FieldType(id) ON DELETE CASCADE,
    UNIQUE(itemId, fieldTypeId)
);

-- История изменений элементов
CREATE TABLE ItemHistory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemId INTEGER NOT NULL,
    userId INTEGER NOT NULL,
    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    diff TEXT, -- JSON с изменениями
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE
);

-- История изменения состояний элементов пользователями
CREATE TABLE ItemUserState (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemId INTEGER NOT NULL,
    userId INTEGER NOT NULL,
    stateId INTEGER NOT NULL,
    comment TEXT,
    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE
);


-- 6. Связи между элементами
-- =====================================================

-- Типы связей между элементами
CREATE TABLE LinkType (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sourceItemTypeId INTEGER NOT NULL,
    destinationItemTypeId INTEGER NOT NULL,
    isBidirectional INTEGER NOT NULL DEFAULT 0,
    caption TEXT NOT NULL,
    FOREIGN KEY (sourceItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
    FOREIGN KEY (destinationItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE
);

-- Связи между элементами
CREATE TABLE ItemLink (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    linkTypeId INTEGER NOT NULL,
    sourceItemId INTEGER NOT NULL,
    destinationItemId INTEGER NOT NULL,
    FOREIGN KEY (linkTypeId) REFERENCES LinkType(id) ON DELETE CASCADE,
    FOREIGN KEY (sourceItemId) REFERENCES Item(id) ON DELETE CASCADE,
    FOREIGN KEY (destinationItemId) REFERENCES Item(id) ON DELETE CASCADE,
    UNIQUE(linkTypeId, sourceItemId, destinationItemId)
);


-- 7. Документы
-- =====================================================

-- Документы
CREATE TABLE Document (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    caption TEXT NOT NULL,
    description TEXT,
    path TEXT NOT NULL,
    filename TEXT NOT NULL,
    size INTEGER NOT NULL,
    mimeType TEXT,
    uploadedAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    uploadedByUserId INTEGER NOT NULL,
    searchCaption TEXT,
    searchDescription TEXT,
    FOREIGN KEY (uploadedByUserId) REFERENCES User(id) ON DELETE CASCADE
);

-- Связь элементов с документами
CREATE TABLE ItemDocument (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemId INTEGER NOT NULL,
    documentId INTEGER NOT NULL,
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
    FOREIGN KEY (documentId) REFERENCES Document(id) ON DELETE CASCADE,
    UNIQUE(itemId, documentId)
);


-- 8. Комментарии
-- =====================================================

-- Комментарии
CREATE TABLE Comment (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    itemId INTEGER NOT NULL,
    createdAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    content TEXT NOT NULL,
    searchContent TEXT,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE
);

-- Документы комментариев
CREATE TABLE CommentDocument (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    commentId INTEGER NOT NULL,
    documentId INTEGER NOT NULL,
    FOREIGN KEY (commentId) REFERENCES Comment(id) ON DELETE CASCADE,
    FOREIGN KEY (documentId) REFERENCES Document(id) ON DELETE CASCADE,
    UNIQUE(commentId, documentId)
);


-- 9. Доски (Kanban)
-- =====================================================

-- Доски
CREATE TABLE Board (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflowId INTEGER NOT NULL,
    projectId INTEGER NOT NULL,
    phaseId INTEGER,
    caption TEXT NOT NULL,
    description TEXT,
    FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE,
    FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,
    FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
);

-- Колонки досок
CREATE TABLE BoardColumn (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    boardId INTEGER NOT NULL,
    stateId INTEGER NOT NULL,
    orderNumber INTEGER DEFAULT 0,
    settings TEXT,
    FOREIGN KEY (boardId) REFERENCES Board(id) ON DELETE CASCADE,
    FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
    UNIQUE(boardId, stateId)
);


-- 10. Рабочее время
-- =====================================================

-- Стандартные рабочие дни (нормальная рабочая неделя)
CREATE TABLE StandardDay (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    weekDayNumber INTEGER NOT NULL CHECK (weekDayNumber BETWEEN 0 AND 6),
    weekOrder INTEGER DEFAULT 0,
    isWorkDay INTEGER NOT NULL DEFAULT 1,
    beginWorkTime TEXT,
    endWorkTime TEXT,
    breakDuration INTEGER DEFAULT 0
);

-- Особые дни (праздники, перенесённые выходные)
CREATE TABLE SpecialDay (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date INTEGER NOT NULL UNIQUE, -- дата в формате timestamp
    isWorkDay INTEGER NOT NULL,
    beginWorkTime TEXT,
    endWorkTime TEXT,
    breakDuration INTEGER DEFAULT 0
);

-- Пользовательские дни (отпуск, больничный)
CREATE TABLE UserDay (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    date INTEGER NOT NULL, -- дата в формате timestamp
    isWorkDay INTEGER NOT NULL,
    beginWorkTime TEXT,
    endWorkTime TEXT,
    breakDuration INTEGER DEFAULT 0,
    description TEXT,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
    UNIQUE(userId, date)
);


-- 11. Сообщения и уведомления
-- =====================================================

-- Личные сообщения
CREATE TABLE PrivateMessage (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    senderUserId INTEGER NOT NULL,
    receiverUserId INTEGER NOT NULL,
    creationTimestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    content TEXT NOT NULL,
    isViewed INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (senderUserId) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY (receiverUserId) REFERENCES User(id) ON DELETE CASCADE
);

-- Сообщения в командах
CREATE TABLE TeamMessage (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    senderUserId INTEGER NOT NULL,
    teamId INTEGER NOT NULL,
    creationTimestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    content TEXT NOT NULL,
    FOREIGN KEY (senderUserId) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE
);

-- Уведомления (подписки на изменения элементов)
CREATE TABLE UserNotification (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    itemId INTEGER NOT NULL,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
    UNIQUE(userId, itemId)
);


-- 12. Действия и задачи
-- =====================================================

-- Действия пользователя (аудит)
CREATE TABLE UserAction (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    caption TEXT NOT NULL,
    description TEXT,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE
);

-- Задачи пользователя (Todo)
CREATE TABLE UserTodo (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    isDone INTEGER NOT NULL DEFAULT 0,
    caption TEXT NOT NULL,
    searchCaption TEXT,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE
);


-- 13. Планирование
-- =====================================================

-- Таблица планов
CREATE TABLE Plan (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    phaseId INTEGER NOT NULL,
    basePlanId INTEGER,
    caption TEXT NOT NULL,
    description TEXT,
    isActive INTEGER NOT NULL DEFAULT 0,
    createdAt INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    createdByUserId INTEGER NOT NULL,
    activatedAt INTEGER,
    activatedByUserId INTEGER,
    FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE CASCADE,
    FOREIGN KEY (basePlanId) REFERENCES Plan(id) ON DELETE SET NULL,
    FOREIGN KEY (createdByUserId) REFERENCES User(id),
    FOREIGN KEY (activatedByUserId) REFERENCES User(id)
);

-- Таблица плановых элементов
CREATE TABLE PlanItem (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    itemId INTEGER NOT NULL,
    userId INTEGER,
    planId INTEGER NOT NULL,
    startDate INTEGER NOT NULL,
    endDate INTEGER NOT NULL,
    FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
    FOREIGN KEY (userId) REFERENCES User(id) ON DELETE SET NULL,
    FOREIGN KEY (planId) REFERENCES Plan(id) ON DELETE CASCADE,
    UNIQUE(planId, itemId)
);

-- =====================================================
-- Индексы для оптимизации запросов
-- =====================================================

CREATE INDEX idx_item_itemTypeId ON Item(itemTypeId);
CREATE INDEX idx_item_parentId ON Item(parentId);
CREATE INDEX idx_item_stateId ON Item(stateId);
CREATE INDEX idx_item_phaseId ON Item(phaseId);
CREATE INDEX idx_item_isDeleted ON Item(isDeleted);

CREATE INDEX idx_itemField_itemId ON ItemField(itemId);
CREATE INDEX idx_itemField_fieldTypeId ON ItemField(fieldTypeId);
CREATE INDEX idx_itemField_searchValue ON ItemField(searchValue);

CREATE INDEX idx_comment_itemId ON Comment(itemId);
CREATE INDEX idx_comment_userId ON Comment(userId);

CREATE INDEX idx_itemHistory_itemId ON ItemHistory(itemId);
CREATE INDEX idx_itemHistory_userId ON ItemHistory(userId);
CREATE INDEX idx_itemHistory_timestamp ON ItemHistory(timestamp);

CREATE INDEX idx_itemUserState_itemId ON ItemUserState(itemId);
CREATE INDEX idx_itemUserState_userId ON ItemUserState(userId);

CREATE INDEX idx_userTeamRole_userId ON UserTeamRole(userId);
CREATE INDEX idx_userTeamRole_teamId ON UserTeamRole(teamId);
CREATE INDEX idx_userTeamRole_roleId ON UserTeamRole(roleId);

CREATE INDEX idx_ruleProject_ruleId ON RuleProject(ruleId);
CREATE INDEX idx_ruleProject_projectId ON RuleProject(projectId);

CREATE INDEX idx_ruleItemType_ruleId ON RuleItemType(ruleId);
CREATE INDEX idx_ruleItemType_itemTypeId ON RuleItemType(itemTypeId);

CREATE INDEX idx_ruleState_ruleId ON RuleState(ruleId);
CREATE INDEX idx_ruleState_stateId ON RuleState(stateId);

CREATE INDEX idx_plan_phaseId ON Plan(phaseId);
CREATE INDEX idx_plan_basePlanId ON Plan(basePlanId);
CREATE INDEX idx_plan_isActive ON Plan(isActive);
CREATE INDEX idx_plan_createdByUserId ON Plan(createdByUserId);
CREATE INDEX idx_plan_activatedByUserId ON Plan(activatedByUserId);

CREATE INDEX idx_planItem_planId ON PlanItem(planId);
CREATE INDEX idx_planItem_itemId ON PlanItem(itemId);
CREATE INDEX idx_planItem_userId ON PlanItem(userId);
CREATE INDEX idx_planItem_dates ON PlanItem(startDate, endDate);
