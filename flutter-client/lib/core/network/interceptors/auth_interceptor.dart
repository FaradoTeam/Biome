import 'package:dio/dio.dart';
import '../../../core/storage/secure_storage.dart';

class AuthInterceptor extends Interceptor {
  @override
  void onRequest(
      RequestOptions options, RequestInterceptorHandler handler) async {
    // Не добавляем токен для запросов логина
    if (options.path.contains('/auth/login')) {
      handler.next(options);
      return;
    }

    final storage = SecureStorage();
    final token = await storage.getToken();
    if (token != null && token.isNotEmpty) {
      options.headers['Authorization'] = 'Bearer $token';
    }
    handler.next(options);
  }
}
