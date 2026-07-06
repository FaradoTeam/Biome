import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../../features/auth/presentation/providers/auth_provider.dart';
import '../../features/auth/presentation/screens/login_screen.dart';
import '../../features/auth/presentation/screens/home_screen.dart';

final routerProvider = Provider<GoRouter>((ref) {
  final authState = ref.watch(authProvider);

  return GoRouter(
    initialLocation: '/login',
    debugLogDiagnostics: true,
    routes: [
      GoRoute(
        path: '/login',
        builder: (context, state) => const LoginScreen(),
      ),
      GoRoute(
        path: '/home',
        builder: (context, state) => const HomeScreen(),
        redirect: (context, state) {
          final isAuthenticated = authState is AuthStateAuthenticated;
          return isAuthenticated ? null : '/login';
        },
      ),
    ],
    redirect: (context, state) {
      final isAuthenticated = authState is AuthStateAuthenticated;
      final isLoginRoute = state.matchedLocation == '/login';
      if (isAuthenticated && isLoginRoute) {
        return '/home';
      }
      if (!isAuthenticated && !isLoginRoute) {
        return '/login';
      }
      return null;
    },
  );
});
