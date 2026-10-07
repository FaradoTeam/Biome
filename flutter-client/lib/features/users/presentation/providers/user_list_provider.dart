import 'dart:async';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../../data/models/user.dart';
import '../../../../data/models/page_response.dart';
import '../../../../domain/repositories/user_repository.dart';
import '../../../auth/presentation/providers/auth_provider.dart';

/// Sentinel для copyWith, чтобы можно было сбрасывать значения в null.
const Object _sentinel = Object();

class UserListState {
  final List<User> users;
  final int currentPage;
  final int totalPages;
  final int totalCount;
  final int pageSize;
  final bool isLoading;
  final String? error;
  final int? idFilter;
  final String? loginFilter;
  final String? nameFilter;
  final String? emailFilter;
  final bool? isBlockedFilter;
  final String? sortField;
  final bool sortAscending;

  UserListState({
    this.users = const [],
    this.currentPage = 1,
    this.totalPages = 1,
    this.totalCount = 0,
    this.pageSize = 20,
    this.isLoading = false,
    this.error,
    this.idFilter,
    this.loginFilter,
    this.nameFilter,
    this.emailFilter,
    this.isBlockedFilter,
    this.sortField,
    this.sortAscending = true,
  });

  UserListState copyWith({
    List<User>? users,
    int? currentPage,
    int? totalPages,
    int? totalCount,
    int? pageSize,
    bool? isLoading,
    Object? error = _sentinel,
    Object? idFilter = _sentinel,
    Object? loginFilter = _sentinel,
    Object? nameFilter = _sentinel,
    Object? emailFilter = _sentinel,
    Object? isBlockedFilter = _sentinel,
    Object? sortField = _sentinel,
    bool? sortAscending,
  }) {
    return UserListState(
      users: users ?? this.users,
      currentPage: currentPage ?? this.currentPage,
      totalPages: totalPages ?? this.totalPages,
      totalCount: totalCount ?? this.totalCount,
      pageSize: pageSize ?? this.pageSize,
      isLoading: isLoading ?? this.isLoading,
      error: identical(error, _sentinel) ? this.error : error as String?,
      idFilter:
          identical(idFilter, _sentinel) ? this.idFilter : idFilter as int?,
      loginFilter: identical(loginFilter, _sentinel)
          ? this.loginFilter
          : loginFilter as String?,
      nameFilter:
          identical(nameFilter, _sentinel) ? this.nameFilter : nameFilter as String?,
      emailFilter: identical(emailFilter, _sentinel)
          ? this.emailFilter
          : emailFilter as String?,
      isBlockedFilter: identical(isBlockedFilter, _sentinel)
          ? this.isBlockedFilter
          : isBlockedFilter as bool?,
      sortField: identical(sortField, _sentinel)
          ? this.sortField
          : sortField as String?,
      sortAscending: sortAscending ?? this.sortAscending,
    );
  }

  bool get hasFilters {
    return idFilter != null ||
        (loginFilter != null && loginFilter!.isNotEmpty) ||
        (nameFilter != null && nameFilter!.isNotEmpty) ||
        (emailFilter != null && emailFilter!.isNotEmpty) ||
        isBlockedFilter != null;
  }
}

class UserListNotifier extends StateNotifier<UserListState> {
  final UserRepository _userRepository;
  Timer? _debounce;

  UserListNotifier(this._userRepository) : super(UserListState());

  @override
  void dispose() {
    _debounce?.cancel();
    super.dispose();
  }

  Future<void> loadUsers({
    int? page,
    int? id,
    String? login,
    String? name,
    String? email,
    bool? isBlocked,
    String? sortField,
    bool? sortAscending,
  }) async {
    // Если поле не передано — используем то, что уже сохранено в state.
    final effectiveId = identical(id, null) ? state.idFilter : id;
    final effectiveLogin = identical(login, null) ? state.loginFilter : login;
    final effectiveName = identical(name, null) ? state.nameFilter : name;
    final effectiveEmail = identical(email, null) ? state.emailFilter : email;
    final effectiveIsBlocked =
        identical(isBlocked, null) ? state.isBlockedFilter : isBlocked;
    final effectiveSortField =
        identical(sortField, null) ? state.sortField : sortField;
    final effectiveSortAscending =
        identical(sortAscending, null) ? state.sortAscending : sortAscending;

    state = state.copyWith(
      isLoading: true,
      error: null,
      idFilter: effectiveId,
      loginFilter: effectiveLogin,
      nameFilter: effectiveName,
      emailFilter: effectiveEmail,
      isBlockedFilter: effectiveIsBlocked,
      sortField: effectiveSortField,
      sortAscending: effectiveSortAscending,
    );

    final currentPage = page ?? state.currentPage;

    try {
      final PageResponse<User> response = await _userRepository.getUsers(
        page: currentPage,
        pageSize: state.pageSize,
        id: effectiveId,
        login: effectiveLogin,
        name: effectiveName,
        email: effectiveEmail,
        isBlocked: effectiveIsBlocked,
        sortField: effectiveSortField,
        sortAscending: effectiveSortAscending,
      );

      final int totalPages =
          response.totalCount == 0 ? 1 : (response.totalCount / state.pageSize).ceil();

      state = state.copyWith(
        users: response.items,
        totalCount: response.totalCount,
        totalPages: totalPages,
        currentPage: currentPage,
        isLoading: false,
        error: null,
      );
    } catch (e) {
      state = state.copyWith(
        isLoading: false,
        error: e.toString(),
      );
    }
  }

  /// Смена сортировки. Данные перезагружаются с сервера.
  void sortBy(String field, bool ascending) {
    state = state.copyWith(
      sortField: field,
      sortAscending: ascending,
      currentPage: 1,
    );
    loadUsers(page: 1);
  }

  /// Установка фильтра с дебаунсом. Значение пустое → сброс фильтра.
  void applyColumnFilter(String column, String value) {
    _debounce?.cancel();
    _debounce = Timer(const Duration(milliseconds: 400), () {
      _applyColumnFilterNow(column, value);
    });
  }

  void _applyColumnFilterNow(String column, String value) {
    final String trimmed = value.trim();

    switch (column) {
      case 'id':
        final int? intValue = int.tryParse(trimmed);
        state = state.copyWith(idFilter: intValue, currentPage: 1);
        break;
      case 'login':
        state = state.copyWith(
          loginFilter: trimmed.isEmpty ? null : trimmed,
          currentPage: 1,
        );
        break;
      case 'name':
        state = state.copyWith(
          nameFilter: trimmed.isEmpty ? null : trimmed,
          currentPage: 1,
        );
        break;
      case 'email':
        state = state.copyWith(
          emailFilter: trimmed.isEmpty ? null : trimmed,
          currentPage: 1,
        );
        break;
      case 'status':
        if (trimmed.isEmpty) {
          state = state.copyWith(isBlockedFilter: null, currentPage: 1);
        } else if (trimmed.toLowerCase().contains('заблок')) {
          state = state.copyWith(isBlockedFilter: true, currentPage: 1);
        } else if (trimmed.toLowerCase().contains('актив')) {
          state = state.copyWith(isBlockedFilter: false, currentPage: 1);
        } else {
          return;
        }
        break;
      default:
        return;
    }

    loadUsers(page: 1);
  }

  void changePage(int newPage) {
    if (newPage < 1 || newPage > state.totalPages) return;
    if (newPage == state.currentPage) return;
    state = state.copyWith(currentPage: newPage);
    loadUsers(page: newPage);
  }

  void clearFilters() {
    _debounce?.cancel();
    state = state.copyWith(
      currentPage: 1,
      idFilter: null,
      loginFilter: null,
      nameFilter: null,
      emailFilter: null,
      isBlockedFilter: null,
    );
    loadUsers(page: 1);
  }
}

final userListProvider =
    StateNotifierProvider<UserListNotifier, UserListState>((ref) {
  final repository = ref.watch(userRepositoryProvider);
  return UserListNotifier(repository);
});
