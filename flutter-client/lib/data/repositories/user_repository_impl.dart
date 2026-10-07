import '../../data/datasources/remote/user_api.dart';
import '../../data/models/page_response.dart';
import '../../data/models/user.dart';
import '../../domain/repositories/user_repository.dart';

class UserRepositoryImpl implements UserRepository {
  final UserApi _userApi;

  UserRepositoryImpl(this._userApi);

  @override
  Future<PageResponse<User>> getUsers({
    required int page,
    required int pageSize,
    int? id,
    String? login,
    String? name,
    String? email,
    bool? isBlocked,
    String? sortField,
    bool? sortAscending,
  }) {
    return _userApi.getUsers(
      page: page,
      pageSize: pageSize,
      id: id,
      login: login,
      name: name,
      email: email,
      isBlocked: isBlocked,
      sortField: sortField,
      sortAscending: sortAscending,
    );
  }
}
