import '../../data/models/page_response.dart';
import '../../data/models/user.dart';

abstract class UserRepository {
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
  });
}