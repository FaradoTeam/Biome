import 'package:json_annotation/json_annotation.dart';

part 'user.g.dart';

@JsonSerializable()
class User {
  final int? id;
  final String? login;
  final String? firstName;
  final String? middleName;
  final String? lastName;
  final String? email;
  final bool? needChangePassword;
  final bool? isBlocked;
  final bool? isSuperAdmin;
  final bool? isHidden;

  User({
    this.id,
    this.login,
    this.firstName,
    this.middleName,
    this.lastName,
    this.email,
    this.needChangePassword,
    this.isBlocked,
    this.isSuperAdmin,
    this.isHidden,
  });

  factory User.fromJson(Map<String, dynamic> json) => _$UserFromJson(json);
  Map<String, dynamic> toJson() => _$UserToJson(this);
}
