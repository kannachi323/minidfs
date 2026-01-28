-- +goose Up
-- +goose StatementBegin
ALTER TABLE ms_tokens ADD COLUMN display_name TEXT DEFAULT '';
ALTER TABLE ms_tokens ADD COLUMN email TEXT DEFAULT '';
-- +goose StatementEnd

-- +goose Down
-- +goose StatementBegin
ALTER TABLE ms_tokens DROP COLUMN display_name;
ALTER TABLE ms_tokens DROP COLUMN email;
-- +goose StatementEnd
