import 'package:flutter_test/flutter_test.dart';
import 'package:anychat_sdk/anychat_sdk.dart';

void main() {
  test('Package exports are accessible', () {
    // Test that the main exports are available
    expect(AnyChatClient, isNotNull);
    expect(ConnectionState, isNotNull);
    expect(MessageType, isNotNull);
    expect(ConversationType, isNotNull);
  });

  test('Enums have expected values', () {
    expect(ConnectionState.values.length, 4);
    expect(MessageType.values.length, 5);
    expect(ConversationType.values.length, 2);
  });
}
