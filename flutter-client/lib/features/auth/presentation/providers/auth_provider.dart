import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:equatable/equatable.dart';

import '../../../../domain/repositories/auth_repository.dart';
import '../../../../data/repositories/auth_repository_impl.dart';
import '../../../../core/network/dio_client.dart';
import '../../../../core/storage/secure_storage.dart';
import '../../../../data/datasources/remote/auth_api.dart';

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

  AuthNotifier(this._authRepository) : super(const AuthStateInitial());

  Future<void> checkAuthStatus() async {
    final isAuth = await _authRepository.isAuthenticated();
    if (isAuth) {
      state = const AuthStateAuthenticated();
    } else {
      state = const AuthStateUnauthenticated();
    }
  }

  Future<void> login(String username, String password) async {
    state = const AuthStateLoading();
    try {
      await _authRepository.login(username, password);
      // После сохранения токена принудительно проверяем статус
      await checkAuthStatus();
    } catch (e) {
      print('AuthNotifier.login: error: $e');
      state = AuthStateError(e.toString());
    }
  }

  Future<void> logout() async {
    state = const AuthStateLoading();
    try {
      await _authRepository.logout();
      state = const AuthStateUnauthenticated();
    } catch (e) {
      state = AuthStateError(e.toString());
    }
  }
}

final authProvider = StateNotifierProvider<AuthNotifier, AuthState>((ref) {
  final repository = ref.watch(authRepositoryProvider);
  return AuthNotifier(repository);
});
