import 'package:dio/dio.dart';

import '../../models/auth/auth_request.dart';
import '../../models/auth/auth_response.dart';

class AuthApi {
  final Dio _dio;

  AuthApi(this._dio);

  Future<AuthResponse> login(AuthRequest request) async {
    try {
      final response = await _dio.post(
        '/auth/login',
        data: request.toJson(),
      );
      return AuthResponse.fromJson(response.data);
    } on DioException catch (e) {
      throw Exception('Ошибка входа: ${e.message}');
    }
  }

  Future<void> logout() async {
    try {
      await _dio.post('/auth/logout');
    } on DioException catch (e) {
      throw Exception('Ошибка выхода: ${e.message}');
    }
  }
}
