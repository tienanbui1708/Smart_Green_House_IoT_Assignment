# cloud/notes.md – Hướng dẫn import và cấu hình CoreIOT

> File này ghi lại các bước import dashboard/rulechain đã xuất và cấu hình notification.
> Cập nhật sau mỗi thay đổi trên CoreIOT (M4, M8).

## Trạng thái (cập nhật theo tiến độ)

- [ ] dashboard.json – chưa có (M4)
- [ ] rulechain_node1.json – chưa có (M4)
- [ ] rulechain_node2.json – chưa có (M4)

## Quy ước đặt tên (KHÔNG thay đổi, firmware và RuleChain phụ thuộc vào đây)

| Đối tượng | Tên |
|---|---|
| Device Node 1 | `Node1` |
| Device Node 2 | `Node2` |
| Device profile Node 1 | `Profile_Node1` |
| Device profile Node 2 | `Profile_Node2` |
| Rule chain Node 1 | `RC_Node1_Ingest` |
| Rule chain Node 2 | `RC_Node2_Command` |
| Dashboard | `Smart Greenhouse` |

## Cách import dashboard

1. Dashboards → biểu tượng **Import** → chọn `dashboard.json`.
2. Sau import, kiểm tra entity alias: `Node1` → device `Node1`, `Node2` → device `Node2`.
3. Kiểm tra widget RPC: method name phải khớp `protocol.h`.

## Cách import rule chain

1. Rule chains → biểu tượng **Import** → chọn `rulechain_node1.json`.
2. Gắn vào profile: Device profiles → `Profile_Node1` → Default rule chain = `RC_Node1_Ingest`.
3. Làm tương tự với `rulechain_node2.json` → `Profile_Node2`.

## Notification

- Recipient group: `Nhóm Greenhouse`
- Template: Alarm template, kênh Web + Mobile (nếu có)
- Rule trigger: Alarm (Created/Cleared) + Device activity (Inactive)

## OTA

- Gói firmware: Title = `greenhouse-node1` hoặc `greenhouse-node2`, phiên bản theo `FW_VERSION` trong `config.h`.
- Gán cho device (không gán cho profile để tránh tự động cập nhật hàng loạt khi test).

