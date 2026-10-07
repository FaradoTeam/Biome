import 'package:dio/dio.dart';
import '../../models/user.dart';
import '../../models/page_response.dart';
import '../../../core/network/api_endpoints.dart';

class UserApi {
  final Dio _dio;

  UserApi(this._dio);

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
  }) async {
    try {
      final Map<String, dynamic> queryParams = {
        'page': page,
        'pageSize': pageSize,
      };

      if (id != null && id > 0) queryParams['id'] = id;
      if (login != null && login.isNotEmpty) queryParams['login'] = login;
      if (name != null && name.isNotEmpty) queryParams['name'] = name;
      if (email != null && email.isNotEmpty) queryParams['email'] = email;
      if (isBlocked != null) queryParams['isBlocked'] = isBlocked;
      if (sortField != null && sortField.isNotEmpty) {
        queryParams['sortField'] = sortField;
        queryParams['sortAscending'] = sortAscending ?? true;
      }

      final response = await _dio.get(
        ApiEndpoints.users,
        queryParameters: queryParams,
      );

      return PageResponse<User>.fromJson(
        response.data,
        (json) => User.fromJson(json as Map<String, dynamic>),
      );
    } on DioException catch (e) {
      throw Exception('Ошибка получения списка пользователей: ${e.message}');
    }
  }
}
