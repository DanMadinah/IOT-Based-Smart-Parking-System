# REST API Reference

The ESP32 hosts both the web UI and a lightweight JSON REST API on port `80`.

## Web Pages

| Method | Path | Auth | Description |
| --- | --- | --- | --- |
| `GET` | `/` | Public | Live driver dashboard (EN / BM, dark/light theme) |
| `GET` | `/about` | Public | Project overview & system limitations |
| `GET` | `/admin` | HTTP Basic (`admin`) | Admin control panel & demo runner |
| `GET` | `/download.csv` | HTTP Basic (`admin`) | Export event log & summary metrics as CSV |

## Public Endpoints

| Method | Endpoint | Query Params | Description |
| --- | --- | --- | --- |
| `GET` | `/api/status` | — | Live lot occupancy, sensor states, gate status, health warnings & recent events |
| `GET` | `/api/history` | — | Last 30 minutes of occupancy samples (20s intervals) |
| `GET` | `/api/pay` | `method`, `name` | Creates a demo RM2 payment & reserves a space; returns ticket `token` |
| `GET` | `/api/tickets` | `t` (comma-separated tokens) | Returns current state & countdown timers for the driver's tickets |
| `GET` | `/api/arm` | `t` | Arms the entry gate ("I'm at the gate") for a reserved ticket |
| `GET` | `/api/code` | `t`, `c` | Submits the 4-digit LCD security code when Secure Mode is active |
| `GET` | `/api/cancel` | `t` | Cancels an active reservation and records a demo refund |
| `GET` | `/api/exitarm` | `t` | Arms the exit gate ("I'm exiting") for a parked ticket |

## Admin Endpoints (HTTP Basic Auth)

| Method | Endpoint | Query Params | Description |
| --- | --- | --- | --- |
| `GET` | `/api/alerts` | — | Security alert counters, revenue summary & recent alert list |
| `GET` | `/api/admin` | `cmd`, `v` | Executes admin commands (`manual`, `gate`, `reset`, `clearstats`, `clearalerts`, `cleartail`, `approveexit`, `mode`, `mute`, `cap`, `buzz`, `sim`, `demo`, `restart`) |
