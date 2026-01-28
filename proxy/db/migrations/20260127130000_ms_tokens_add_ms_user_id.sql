-- +goose Up
-- +goose StatementBegin
-- Support multiple MS accounts per app user. Track by Microsoft Graph user id (ms_user_id).
CREATE TABLE IF NOT EXISTS ms_tokens_new (
    user_id TEXT NOT NULL,
    ms_user_id TEXT NOT NULL,
    access_token TEXT NOT NULL,
    refresh_token TEXT,
    PRIMARY KEY (user_id, ms_user_id)
);

-- Migrate existing rows: use user_id as ms_user_id placeholder so we keep at most one legacy row per user.
INSERT OR IGNORE INTO ms_tokens_new (user_id, ms_user_id, access_token, refresh_token)
SELECT user_id, user_id, access_token, refresh_token FROM ms_tokens;

DROP TABLE ms_tokens;
ALTER TABLE ms_tokens_new RENAME TO ms_tokens;
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
CREATE TABLE IF NOT EXISTS ms_tokens_old (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id TEXT NOT NULL,
    access_token TEXT NOT NULL,
    refresh_token TEXT
);

INSERT INTO ms_tokens_old (user_id, access_token, refresh_token)
SELECT user_id, access_token, refresh_token FROM ms_tokens;

DROP TABLE ms_tokens;
ALTER TABLE ms_tokens_old RENAME TO ms_tokens;
-- +goose StatementEnd
