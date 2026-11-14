-- Preserve existing subscriptions and supply the default text handler.
-- Select the intended game database explicitly before applying.
ALTER TABLE pubsub_subscriptions
  ADD COLUMN IF NOT EXISTS handler_name VARCHAR(80) NOT NULL
  DEFAULT 'send_text' AFTER player_name;
