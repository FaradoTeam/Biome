import 'package:dio/dio.dart';

import 'interceptors/auth_interceptor.dart';
import 'interceptors/logging_interceptor.dart';
import 'api_endpoints.dart';
import '../storage/secure_storage.dart';

// Глобальный колбэк для обработки 401 ошибок
typedef OnUnauthorizedCallback = void Function();

class DioClient {
  static Dio? _instance;
  static OnUnauthorizedCallback? _onUnauthorized;

  static void setOnUnauthorized(OnUnauthorizedCallback callback) {
    _onUnauthorized = callback;
  }

  static Dio get instance {
    _instance ??= _createDio();
    return _instance!;
  }

  static Dio _createDio() {
    final dio = Dio(
      BaseOptions(
        baseUrl: ApiEndpoints.baseUrl,
        connectTimeout: const Duration(seconds: 30),
        receiveTimeout: const Duration(seconds: 30),
        contentType: 'application/json',
      ),
    );

    // Добавляем AuthInterceptor
    dio.interceptors.add(AuthInterceptor());

    // Добавляем обработку 401 ошибок
    dio.interceptors.add(
      InterceptorsWrapper(
        onError: (DioException error, ErrorInterceptorHandler handler) async {
          if (error.response?.statusCode == 401) {
            // Очищаем токен
            await SecureStorage().deleteToken();
            // Вызываем колбэк если он установлен
            if (_onUnauthorized != null) {
              _onUnauthorized!();
            }
          }
          handler.next(error);
        },
      ),
    );

    // Для отладки можно включить логирование
    if (const bool.fromEnvironment('DEBUG_MODE')) {
      dio.interceptors.add(LoggingInterceptor());
    }

    return dio;
  }
}
