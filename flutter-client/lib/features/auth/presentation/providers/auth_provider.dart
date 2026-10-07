import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:equatable/equatable.dart';

import '../../../../domain/repositories/auth_repository.dart';
import '../../../../data/repositories/auth_repository_impl.dart';
import '../../../../core/network/dio_client.dart';
import '../../../../core/storage/secure_storage.dart';
import '../../../../data/datasources/remote/auth_api.dart';
import '../../../../data/datasources/remote/user_api.dart';
import '../../../../data/repositories/user_repository_impl.dart';
import '../../../../domain/repositories/user_repository.dart';

// ---- Providers for dependencies ----
final secureStorageProvider = Provider<SecureStorage>((ref) => SecureStorage());

final authApiProvider = Provider<AuthApi>((ref) {
  final dio = DioClient.instance;
  return AuthApi(dio);
});

final authRepositoryProvider = Provider<AuthRepository>((ref) {
  final api = ref.watch(authApiProvider);
  final storage = ref.watch(secureStorageProvider);
  return AuthRepositoryImpl(api, storage);
});

// ---- Providers for User ----
final userApiProvider = Provider<UserApi>((ref) {
  final dio = DioClient.instance;
  return UserApi(dio);
});

final userRepositoryProvider = Provider<UserRepository>((ref) {
  final api = ref.watch(userApiProvider);
  return UserRepositoryImpl(api);
});

// ---- State ----
abstract class AuthState extends Equatable {
  const AuthState();
  @override
  List<Object?> get props => [];
}

class AuthStateInitial extends AuthState {
  const AuthStateInitial();
}

class AuthStateLoading extends AuthState {
  const AuthStateLoading();
}

class AuthStateAuthenticated extends AuthState {
  const AuthStateAuthenticated();
}

class AuthStateUnauthenticated extends AuthState {
  const AuthStateUnauthenticated();
}

class AuthStateError extends AuthState {
  final String message;
  const AuthStateError(this.message);
  @override
  List<Object?> get props => [message];
}

// ---- Notifier ----
class AuthNotifier extends StateNotifier<AuthState> {
  final AuthRepository _authRepository;

  AuthNotifier(this._authRepository) : super(const AuthStateInitial()) {
    // Автоматически проверяем статус при создании
    checkAuthStatus();
  }

  Future<void> checkAuthStatus() async {
    try {
      final isAuth = await _authRepository.isAuthenticated();
      if (isAuth) {
        state = const AuthStateAuthenticated();
      } else {
        state = const AuthStateUnauthenticated();
      }
    } catch (e) {
      await _authRepository.logout();
      state = const AuthStateUnauthenticated();
    }
  }

  Future<void> login(String username, String password) async {
    state = const AuthStateLoading();
    try {
      await _authRepository.login(username, password);
      state = const AuthStateAuthenticated();
    } catch (e) {
      state = AuthStateError(e.toString());
    }
  }

  Future<void> logout() async {
    state = const AuthStateLoading();
    try {
      await _authRepository.logout();
      state = const AuthStateUnauthenticated();
    } catch (e) {
      await _authRepository.logout();
      state = const AuthStateUnauthenticated();
    }
  }

  // Метод для обработки 401 ошибки
  void onUnauthorized() {
    if (state is AuthStateAuthenticated) {
      _authRepository.logout();
      state = const AuthStateUnauthenticated();
    }
  }
}

final authProvider = StateNotifierProvider<AuthNotifier, AuthState>((ref) {
  final repository = ref.watch(authRepositoryProvider);
  return AuthNotifier(repository);
});
