import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:flutter_sample/config.dart';
import 'package:flutter_sample/services/auth_service.dart';
import 'package:flutter_sample/screens/welcome_screen.dart';
import 'package:flutter_sample/screens/login_screen.dart';
import 'package:flutter_sample/screens/register_screen.dart';
import 'package:flutter_sample/screens/home_screen.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => AuthService(
        gatewayUrl: AppConfig.gatewayUrl,
        apiBaseUrl: AppConfig.apiBaseUrl,
        deviceId: AppConfig.deviceId,
      ),
      child: MaterialApp(
        title: 'AnyChat Demo',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
          useMaterial3: true,
        ),
        initialRoute: '/',
        routes: {
          '/': (context) => const WelcomeScreen(),
          '/login': (context) => const LoginScreen(),
          '/register': (context) => const RegisterScreen(),
          '/home': (context) => const HomeScreen(),
        },
      ),
    );
  }
}
