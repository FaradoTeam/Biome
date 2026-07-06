import '../../domain/repositories/auth_repository.dart';
import '../datasources/remote/auth_api.dart';
import '../models/auth/auth_request.dart';
import '../../core/storage/secure_storage.dart';

class AuthRepositoryImpl implements AuthRepository {
  final AuthApi _authApi;
  final SecureStorage _secureStorage;

  AuthRepositoryImpl(this._authApi, this._secureStorage);

  @override
  Future<String> login(String username, String password) async {
    final request = AuthRequest(login: username, password: password);
    final response = await _authApi.login(request);
    await _secureStorage.saveToken(response.accessToken);
    return response.accessToken;
  }

  @override
  Future<void> logout() async {
    await _authApi.logout();
    await _secureStorage.deleteToken();
  }

  @override
  Future<String?> getToken() async {
    return await _secureStorage.getToken();
  }

  @override
  Future<bool> isAuthenticated() async {
    final token = await getToken();
    return token != null && token.isNotEmpty;
  }
}
