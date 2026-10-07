import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:pluto_grid_plus/pluto_grid_plus.dart';

import '../../../../core/theme/app_colors.dart';
import '../../../../data/models/user.dart';
import '../providers/user_list_provider.dart';

class UserGridSheet extends ConsumerStatefulWidget {
  const UserGridSheet({super.key});

  @override
  ConsumerState<UserGridSheet> createState() => _UserGridSheetState();
}

class _UserGridSheetState extends ConsumerState<UserGridSheet> {
  late final List<PlutoColumn> _columns;
  PlutoGridStateManager? _stateManager;

  /// Контроллеры, которые выдаёт сам pluto_grid через filterWidgetBuilder.
  /// Нужны только для того, чтобы кнопка «Сбросить» могла очистить поля.
  final Map<String, TextEditingController> _filterControllers = {};

  @override
  void initState() {
    super.initState();
    _columns = _buildColumns();
  }

  @override
  void dispose() {
    // Контроллерами владеет pluto_grid — мы только храним ссылки.
    _filterControllers.clear();
    super.dispose();
  }

  // ---------------------------------------------------------------------------
  // Колонки
  // ---------------------------------------------------------------------------

  List<PlutoColumn> _buildColumns() {
    return <PlutoColumn>[
      PlutoColumn(
        title: 'ID',
        field: 'id',
        type: PlutoColumnType.number(),
        width: 100,
        textAlign: PlutoColumnTextAlign.center,
        enableSorting: true,
        enableColumnDrag: false,
        enableEditingMode: false,
        enableContextMenu: false,
        readOnly: true,
        // Кастомный виджет фильтра для колонки.
        filterWidgetDelegate: PlutoFilterColumnWidgetDelegate.builder(
          filterWidgetBuilder: (focusNode, controller, enabled, handleOnChanged, stateManager) {
            return _buildFilterField('id', 'ID', focusNode, controller, enabled, handleOnChanged);
          },
        ),
      ),
      PlutoColumn(
        title: 'Логин',
        field: 'login',
        type: PlutoColumnType.text(),
        width: 150,
        enableSorting: true,
        enableColumnDrag: false,
        enableEditingMode: false,
        enableContextMenu: false,
        readOnly: true,
        filterWidgetDelegate: PlutoFilterColumnWidgetDelegate.builder(
          filterWidgetBuilder: (focusNode, controller, enabled, handleOnChanged, stateManager) {
            return _buildFilterField('login', 'Логин', focusNode, controller, enabled, handleOnChanged);
          },
        ),
      ),
      PlutoColumn(
        title: 'ФИО',
        field: 'name',
        type: PlutoColumnType.text(),
        width: 250,
        enableSorting: true,
        enableColumnDrag: false,
        enableEditingMode: false,
        enableContextMenu: false,
        readOnly: true,
        filterWidgetDelegate: PlutoFilterColumnWidgetDelegate.builder(
          filterWidgetBuilder: (focusNode, controller, enabled, handleOnChanged, stateManager) {
            return _buildFilterField('name', 'ФИО', focusNode, controller, enabled, handleOnChanged);
          },
        ),
      ),
      PlutoColumn(
        title: 'Email',
        field: 'email',
        type: PlutoColumnType.text(),
        width: 200,
        enableSorting: true,
        enableColumnDrag: false,
        enableEditingMode: false,
        enableContextMenu: false,
        readOnly: true,
        filterWidgetDelegate: PlutoFilterColumnWidgetDelegate.builder(
          filterWidgetBuilder: (focusNode, controller, enabled, handleOnChanged, stateManager) {
            return _buildFilterField('email', 'Email', focusNode, controller, enabled, handleOnChanged);
          },
        ),
      ),
      PlutoColumn(
        title: 'Статус',
        field: 'status',
        type: PlutoColumnType.text(),
        width: 120,
        textAlign: PlutoColumnTextAlign.center,
        enableSorting: true,
        enableColumnDrag: false,
        enableEditingMode: false,
        enableContextMenu: false,
        readOnly: true,
        filterWidgetDelegate: PlutoFilterColumnWidgetDelegate.builder(
          filterWidgetBuilder: (focusNode, controller, enabled, handleOnChanged, stateManager) {
            return _buildFilterField('status', 'Активен/Заблок.', focusNode, controller, enabled, handleOnChanged);
          },
        ),
      ),
      PlutoColumn(
        title: 'Действия',
        field: 'actions',
        type: PlutoColumnType.text(),
        width: 120,
        textAlign: PlutoColumnTextAlign.center,
        enableSorting: false,
        enableColumnDrag: false,
        enableEditingMode: false,
        enableContextMenu: false,
        readOnly: true,
        renderer: (PlutoColumnRendererContext ctx) {
          final user = ctx.cell.value as User?;
          if (user == null) return const SizedBox.shrink();
          return _buildActionsCell(user);
        },
      ),
    ];
  }

  // ---------------------------------------------------------------------------
  // Виджет поля фильтра (используется внутри filterWidgetBuilder)
  // ---------------------------------------------------------------------------

  Widget _buildFilterField(
    String field,
    String hint,
    FocusNode focusNode,
    TextEditingController controller,
    bool enabled,
    void Function(String) handleOnChanged,
  ) {
    // Запоминаем контроллер, чтобы кнопка «Сбросить» могла его очистить.
    _filterControllers[field] = controller;

    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 4),
      child: TextField(
        controller: controller,
        focusNode: focusNode,
        enabled: enabled,
        style: const TextStyle(fontSize: 12),
        decoration: InputDecoration(
          hintText: hint,
          hintStyle: const TextStyle(fontSize: 11, color: AppColors.textHint),
          isDense: true,
          contentPadding: const EdgeInsets.symmetric(horizontal: 6, vertical: 4),
          border: const OutlineInputBorder(
            borderRadius: BorderRadius.zero,
            borderSide: BorderSide(color: AppColors.divider, width: 0.5),
          ),
          enabledBorder: const OutlineInputBorder(
            borderRadius: BorderRadius.zero,
            borderSide: BorderSide(color: AppColors.divider, width: 0.5),
          ),
          focusedBorder: const OutlineInputBorder(
            borderRadius: BorderRadius.zero,
            borderSide: BorderSide(color: AppColors.primary, width: 1),
          ),
        ),
        onChanged: (value) {
          // Сообщаем pluto_grid об изменении. В режиме filterOnlyEvent
          // локальная фильтрация не выполняется, генерируется только событие.
          handleOnChanged(value);
          // Отправляем запрос на сервер.
          ref.read(userListProvider.notifier).applyColumnFilter(field, value);
        },
      ),
    );
  }

  // ---------------------------------------------------------------------------
  // Строки
  // ---------------------------------------------------------------------------

  List<PlutoRow> _buildRows(List<User> users) {
    return users.map((User user) {
      final String name =
          '${user.lastName ?? ''} ${user.firstName ?? ''} ${user.middleName ?? ''}'
              .trim()
              .replaceAll(RegExp(r'\s+'), ' ');

      return PlutoRow(
        cells: <String, PlutoCell>{
          'id': PlutoCell(value: user.id),
          'login': PlutoCell(value: user.login ?? ''),
          'name': PlutoCell(value: name),
          'email': PlutoCell(value: user.email ?? ''),
          'status': PlutoCell(
            value: user.isBlocked == true ? 'Заблок.' : 'Активен',
          ),
          'actions': PlutoCell(value: user),
        },
      );
    }).toList();
  }

  void _syncRows(List<User> users) {
    final sm = _stateManager;
    if (sm == null) return;
    sm.removeAllRows();
    sm.appendRows(_buildRows(users));
  }

  Widget _buildActionsCell(User user) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: <Widget>[
        if (user.isSuperAdmin ?? false)
          const Tooltip(
            message: 'Супер-админ',
            child: Icon(
              Icons.admin_panel_settings,
              size: 14,
              color: AppColors.warning,
            ),
          ),
        if (user.needChangePassword ?? false)
          const Tooltip(
            message: 'Сменить пароль',
            child: Icon(Icons.lock_reset, size: 14, color: AppColors.warning),
          ),
        if (user.isHidden ?? false)
          const Tooltip(
            message: 'Скрыт',
            child: Icon(
              Icons.visibility_off,
              size: 14,
              color: AppColors.grey500,
            ),
          ),
        IconButton(
          icon: const Icon(Icons.edit, size: 14, color: AppColors.primary),
          onPressed: () {
            // TODO: открыть диалог редактирования
          },
          padding: EdgeInsets.zero,
          constraints: const BoxConstraints(),
          tooltip: 'Редактировать',
        ),
      ],
    );
  }

  // ---------------------------------------------------------------------------
  // build
  // ---------------------------------------------------------------------------

  @override
  Widget build(BuildContext context) {
    ref.listen<UserListState>(userListProvider, (prev, next) {
      _syncRows(next.users);
    });

    final state = ref.watch(userListProvider);

    if (state.error != null) {
      return Center(
        child: Text(
          'Ошибка: ${state.error}',
          style: const TextStyle(color: Colors.red),
        ),
      );
    }

    return Column(
      children: <Widget>[
        // Кнопка сброса фильтров и индикатор загрузки
        Container(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
          color: AppColors.backgroundAlt,
          child: Row(
            children: <Widget>[
              const Text(
                'Фильтры:',
                style: TextStyle(fontSize: 12, color: AppColors.textSecondary),
              ),
              const SizedBox(width: 8),
              TextButton.icon(
                onPressed: () {
                  for (final c in _filterControllers.values) {
                    c.clear();
                  }
                  _stateManager?.setFilterWithFilterRows([]);
                  ref.read(userListProvider.notifier).clearFilters();
                },
                icon: const Icon(Icons.clear, size: 16),
                label: const Text('Сбросить', style: TextStyle(fontSize: 12)),
              ),
              const Spacer(),
              if (state.isLoading)
                const SizedBox(
                  width: 16,
                  height: 16,
                  child: CircularProgressIndicator(
                    strokeWidth: 2,
                    color: AppColors.primary,
                  ),
                ),
            ],
          ),
        ),
        Expanded(
          child: PlutoGrid(
            columns: _columns,
            rows: _buildRows(state.users),
            mode: PlutoGridMode.readOnly,
            configuration: PlutoGridConfiguration(
              style: PlutoGridStyleConfig(
                gridBackgroundColor: AppColors.background,
                rowHeight: 40,
                columnHeight: 40,
                gridBorderColor: AppColors.divider,
                borderColor: AppColors.divider,
                activatedColor: AppColors.primaryLight,
                cellTextStyle: const TextStyle(
                  fontSize: 12,
                  color: AppColors.textPrimary,
                ),
                columnTextStyle: const TextStyle(
                  fontWeight: FontWeight.w600,
                  fontSize: 12,
                  color: AppColors.textSecondary,
                ),
              ),
              columnSize: const PlutoGridColumnSizeConfig(
                autoSizeMode: PlutoAutoSizeMode.none,
                resizeMode: PlutoResizeMode.normal,
              ),
              scrollbar: const PlutoGridScrollbarConfig(
                isAlwaysShown: true,
              ),
            ),
            onLoaded: (PlutoGridOnLoadedEvent event) {
              _stateManager = event.stateManager;
              // Показываем строку фильтров под заголовком.
              _stateManager!.setShowColumnFilter(true);
              // Отключаем локальную фильтрацию: pluto_grid только
              // генерирует событие, а фильтрует сервер.
              _stateManager!.setFilterOnlyEvent(true);
              _syncRows(state.users);
            },
            onSorted: (PlutoGridOnSortedEvent event) {
              final String field = event.column.field;
              final bool ascending = event.column.sort.isAscending;
              ref.read(userListProvider.notifier).sortBy(field, ascending);
            },
            noRowsWidget: const Center(
              child: Padding(
                padding: EdgeInsets.all(24),
                child: Text(
                  'Пользователи не найдены',
                  style: TextStyle(
                    color: AppColors.grey600,
                    fontSize: 14,
                  ),
                ),
              ),
            ),
          ),
        ),
        _buildPaginationFooter(state),
      ],
    );
  }

  // ---------------------------------------------------------------------------
  // Футер пагинации
  // ---------------------------------------------------------------------------

  Widget _buildPaginationFooter(UserListState state) {
    final int totalPages = state.totalPages == 0 ? 1 : state.totalPages;
    final int currentPage = state.currentPage;

    return Container(
      height: 40,
      padding: const EdgeInsets.symmetric(horizontal: 12),
      decoration: const BoxDecoration(
        color: AppColors.headerBg,
        border: Border(top: BorderSide(color: AppColors.divider)),
      ),
      child: Row(
        children: <Widget>[
          Text(
            'Всего: ${state.totalCount}',
            style: const TextStyle(
              fontSize: 12,
              color: AppColors.textSecondary,
            ),
          ),
          const Spacer(),
          IconButton(
            icon: const Icon(Icons.chevron_left, size: 18),
            onPressed: currentPage > 1
                ? () => ref
                    .read(userListProvider.notifier)
                    .changePage(currentPage - 1)
                : null,
            tooltip: 'Назад',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Text(
              '$currentPage / $totalPages',
              style: const TextStyle(
                fontSize: 12,
                color: AppColors.textPrimary,
              ),
            ),
          ),
          IconButton(
            icon: const Icon(Icons.chevron_right, size: 18),
            onPressed: currentPage < totalPages
                ? () => ref
                    .read(userListProvider.notifier)
                    .changePage(currentPage + 1)
                : null,
            tooltip: 'Вперёд',
            padding: EdgeInsets.zero,
            constraints: const BoxConstraints(minWidth: 32, minHeight: 32),
          ),
        ],
      ),
    );
  }
}
