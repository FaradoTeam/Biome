import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';

import '../providers/auth_provider.dart';

class HomeScreen extends ConsumerWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final authState = ref.watch(authProvider);

    // Если сессия протухла или ошибка - возвращаем на логин
    if (authState is AuthStateUnauthenticated ||
        authState is AuthStateInitial ||
        authState is AuthStateError) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (context.mounted) {
          ref.read(authProvider.notifier).logout();
        }
      });
      return const Scaffold(
        body: Center(child: CircularProgressIndicator()),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text('Главная'),
        actions: [
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () {
              ref.read(authProvider.notifier).logout();
            },
          ),
        ],
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Text('Добро пожаловать!'),
            const SizedBox(height: 20),
            ElevatedButton.icon(
              onPressed: () {
                context.go('/users');
              },
              icon: const Icon(Icons.people),
              label: const Text('Управление пользователями'),
            ),
          ],
        ),
      ),
    );
  }
}
