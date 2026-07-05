# ex-obscura-npc-generator

A minimal C++ NPC generator designed to run entirely inside Docker Compose.

## Start with Docker Compose

Build and start the container:

```bash
docker compose up --build
```

This maps host port `8080` to container port `8080`.

## Compile inside the container

Start the container in the background:

```powershell
docker compose up --build -d
```

Open a shell inside the running container:

```powershell
docker compose exec npc-generator sh
```

Then compile the project inside the container:

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Run the server inside the container

From the same container shell, start the server:

```sh
./build/server
```

## See the frontend

Once the server is running, open your browser to:

```text
http://localhost:8080/
```

That page serves the new occult-style NPC dossier UI. Click the button to generate a new NPC and view its age, race, subrace, sanity points, occupation, clothing, personality, and secret.

If you want to start the server from the host using Docker Compose again, first stop the container and then run:

```powershell
docker compose up --build
```

## Shortcut commands

- `docker compose up --build -d` — start the container in detached mode
- `docker compose exec npc-generator sh` — open a shell inside the running container
- `docker compose down` — stop the container

## Notes

- You do not need bash or make on Windows; use `sh` inside the container.
- The container source is mounted into `/work`.
- If port `8080` is in use, change the port mapping in `docker-compose.yml`.

## API Endpoints

- `GET /npc/random`
- `POST /npc` with JSON body like:

```json
{ "seed": 42, "world": { "year": 1926, "coastal": true } }
```

## Notes

- The container uses the repository source mounted into `/work`.
- If port `8080` is already in use, change the port mapping in `docker-compose.yml`.
- Names from https://github.com/aruljohn/popular-baby-names/tree/master/1890
- Surnames from https://sites.rootsweb.com/~arwhite2/1890tax.html