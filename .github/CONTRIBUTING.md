# Contribution Guidelines / Guías de Contribución

[🇬🇧 English](#english)  |  [🇪🇸 Español](#español)

---

## Quick start

- **Install hooks:** run `make hook` after cloning. This installs the repository git hooks and enables automatic checks on `git commit`.
- To skip hooks for a single commit use `git commit --no-verify`.

## Inicio rapido

- **Instalar hooks:** ejecuta `make hook` tras clonar. Instala los git hooks del repositorio y activa verificaciones automaticas para `git commit`.
- Para saltar los hooks en un unico commit `git commit --no-verify`.

---

<a name="english"></a>
## 🇬🇧 English

### Git hooks

Run:

```sh
make hook
```

This installs the repository pre-commit hooks into `.git/hooks/` and ensures checks run automatically on commits.

### Commit messages (Conventional Commits)

To maintain a clear commit history, follow the Conventional Commits specification: https://www.conventionalcommits.org/

Format:

```text
<type>(<optional-scope>): <short description>

[optional body]

[optional footer]
```

- **Body:** explain *why* the change was made and provide technical context.
- **Footer:** reference tracking IDs (issues, task numbers), e.g. `Closes #123`.

Allowed types:

- **feat:** new feature
- **fix:** bug fix
- **docs:** documentation only
- **style:** formatting, no logic changes
- **refactor:** code change that neither fixes a bug nor adds a feature
- **test:** tests added or modified
- **chore:** build, tooling, or dependency changes

Golden rules:

1. Use the imperative, NO past tense: `fix: handle nil pointer`
2. Keep the short description lowercase
3. Don’t end the short description with a period

Examples:

Simple:

```text
feat(auth): add password reset flow
```

Full (with body and footer):

```text
fix(api): resolve memory leak during user fetching

The previous implementation kept a DB connection open after every request. Switched to a scoped connection pool to ensure proper release.

Closes #142
```

---

<a name="español"></a>
## 🇪🇸 Español

### Hooks de Git

Ejecuta:

```sh
make hook
```

Esto instala los hooks pre-commit en `.git/hooks/` y asegura que verificaciones se ejecuten automáticamente al hacer commits.

### Mensajes de commit (Conventional Commits)

Para mantener un historial de commits claro y limpio, usa la especificación Conventional Commits: https://www.conventionalcommits.org/es/

Formato:

```text
<tipo>(<alcance opcional>): <descripción corta>

[cuerpo opcional]

[pie de página opcional]
```

- **Cuerpo:** explica *por qué* se hizo el cambio y aporta contexto técnico.
- **Pie de página:** referencia IDs de seguimiento (issues), por ejemplo `Closes #123`.

Tipos permitidos:

- **feat:** nueva funcionalidad
- **fix:** corrección de bug
- **docs:** documentación solamente
- **style:** formato, sin cambios de lógica
- **refactor:** cambio de código que no añade función ni corrige bug
- **test:** pruebas añadidas o modificadas
- **chore:** cambios de build, herramientas o dependencias

Reglas de oro:

1. Usa imperativo, NO el tiempo pasado: `fix: handle nil pointer`
2. Mantén la descripción corta en minúsculas
3. No pongas punto final en la descripción corta

Ejemplos:

Simple:

```text
feat(auth): add password reset flow
```

Completo (con cuerpo y pie de página):

```text
fix(api): resolve memory leak during user fetching

La implementación anterior mantenía una conexión a la BD abierta después de cada petición. Se cambió a un pool con alcance para asegurar la liberación correcta.

Closes #142
```
