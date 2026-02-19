import 'models.dart';

abstract class MessageManager {
  Future<void> sendTextMessage(String sessionId, String content);
  Future<List<Message>> getHistory(String sessionId, {DateTime? before, int limit = 20});
  Future<void> markAsRead(String sessionId, String messageId);
  Stream<Message> get onMessageReceived;
}
