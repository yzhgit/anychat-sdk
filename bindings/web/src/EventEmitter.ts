/**
 * EventEmitter.ts - Simple event emitter implementation
 */

type Listener<T = any> = (data: T) => void;

export class EventEmitter<EventMap extends Record<string, any>> {
  private listeners: Map<keyof EventMap, Set<Listener>> = new Map();

  on<K extends keyof EventMap>(event: K, listener: Listener<EventMap[K]>): void {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, new Set());
    }
    this.listeners.get(event)!.add(listener);
  }

  off<K extends keyof EventMap>(event: K, listener: Listener<EventMap[K]>): void {
    const listeners = this.listeners.get(event);
    if (listeners) {
      listeners.delete(listener);
      if (listeners.size === 0) {
        this.listeners.delete(event);
      }
    }
  }

  once<K extends keyof EventMap>(event: K, listener: Listener<EventMap[K]>): void {
    const onceListener = (data: EventMap[K]) => {
      this.off(event, onceListener);
      listener(data);
    };
    this.on(event, onceListener);
  }

  emit<K extends keyof EventMap>(event: K, data: EventMap[K]): void {
    const listeners = this.listeners.get(event);
    if (listeners) {
      listeners.forEach((listener) => listener(data));
    }
  }

  removeAllListeners<K extends keyof EventMap>(event?: K): void {
    if (event) {
      this.listeners.delete(event);
    } else {
      this.listeners.clear();
    }
  }
}
