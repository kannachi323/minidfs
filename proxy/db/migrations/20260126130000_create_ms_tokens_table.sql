-- +goose Up
-- +goose StatementBegin
CREATE TABLE IF NOT EXISTS ms_tokens (
    user_id TEXT NOT NULL PRIMARY KEY REFERENCES users(id),
    access_token TEXT NOT NULL,
    refresh_token TEXT
);
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
DROP TABLE IF EXISTS ms_tokens;
-- +goose StatementEnd
