// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'user.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

User _$UserFromJson(Map<String, dynamic> json) => User(
      id: (json['id'] as num?)?.toInt(),
      login: json['login'] as String?,
      firstName: json['firstName'] as String?,
      middleName: json['middleName'] as String?,
      lastName: json['lastName'] as String?,
      email: json['email'] as String?,
      needChangePassword: json['needChangePassword'] as bool?,
      isBlocked: json['isBlocked'] as bool?,
      isSuperAdmin: json['isSuperAdmin'] as bool?,
      isHidden: json['isHidden'] as bool?,
    );

Map<String, dynamic> _$UserToJson(User instance) => <String, dynamic>{
      'id': instance.id,
      'login': instance.login,
      'firstName': instance.firstName,
      'middleName': instance.middleName,
      'lastName': instance.lastName,
      'email': instance.email,
      'needChangePassword': instance.needChangePassword,
      'isBlocked': instance.isBlocked,
      'isSuperAdmin': instance.isSuperAdmin,
      'isHidden': instance.isHidden,
    };
