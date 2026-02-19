enum MessageType { text, image, file, audio, video }

class Message {
  final String messageId;
  final String sessionId;
  final String senderId;
  final MessageType type;
  final String content;
  final DateTime timestamp;
  final bool isRead;

  const Message({
    required this.messageId,
    required this.sessionId,
    required this.senderId,
    required this.type,
    required this.content,
    required this.timestamp,
    this.isRead = false,
  });
}
