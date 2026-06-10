# Contribution Guidelines / Guías de Contribución

🇬🇧 [English Version](#english)   |   🇪🇸 [Versión en Español](#español)

---

<a name="english"></a>
## 🇬🇧 English

To keep our change history clean and readable, we follow the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) specification.

### Full Commit Message Format
```text
<type>(<optional-scope>): <short description>

[optional body]

[optional footer]
```

### When to use Body and Footer?
* **Body**: Use it to explain **why** you made the change and provide technical context. Separate it from the description with a blank line.
* **Footer**: Use it to reference tracking IDs, such as closing GitHub issues.

### Allowed Types
* **feat**: A new feature or functionality.
* **fix**: A bug fix.
* **docs**: Changes to documentation only.V
* **style**: Code formatting (spaces, semicolons) without logic changes.
* **refactor**: Code changes that neither fix a bug nor add a feature.
* **test**: Adding or modifying tests.
* **chore**: Updating dependencies, build tools, or configuration.

### Golden Rules
1. **Use imperative**: Write `feat: add feature` instead of `added` or `adds`.
2. **Lowercase**: Do not capitalize the first letter of the short description.
3. **No period**: Do not end the short description with a dot `.`.

### Examples

**Simple Commit:**
```text
feat(auth): add password reset flow
```

**Full Commit (with Body and Footer):**
```text
fix(api): resolve memory leak during user fetching

The previous implementation kept a DB connection open after every request. 
Switched to a scoped connection pool to ensure proper release.

Closes #142
```

---

<a name="español"></a>
## 🇪🇸 Español

Para mantener el historial de cambios limpio y legible, adoptamos la especificación de [Conventional Commits](https://www.conventionalcommits.org/es/v1.0.0/).

### Estructura Completa del Mensaje
```text
<tipo>(<alcance opcional>): <descripción corta>

[cuerpo del mensaje opcional]

[pie de página opcional]
```

### ¿Cuándo usar el Cuerpo y el Pie de Página?
* **Cuerpo (Body)**: Úsalo para explicar **por qué** hiciste el cambio y dar contexto técnico. Sepáralo de la descripción con una línea en blanco.
* **Pie de página (Footer)**: Úsalo para referenciar IDs de seguimiento, como cerrar *issues* de GitHub.

### Tipos Permitidos
* **feat**: Nueva característica o funcionalidad.
* **fix**: Corrección de un error (bug).
* **docs**: Cambios solo en la documentación.
* **style**: Formato de código (espacios, puntos y coma) sin cambios de lógica.
* **refactor**: Cambios en el código que no añaden funciones ni corrigen errores.
* **test**: Añadir o modificar pruebas.
* **chore**: Actualización de dependencias, herramientas de desarrollo o configuración.

### Reglas de Oro
1. **Usa el imperativo**: Escribe `feat: add feature` en lugar de `added` o `adds`.
2. **Minúsculas**: La descripción corta no debe iniciar con mayúscula.
3. **Sin punto**: No pongas punto final `.` al terminar la descripción corta.

### Ejemplos

**Commit Simple:**
```text
feat(auth): add password reset flow
```

**Commit Completo (con Cuerpo y Pie de página):**
```text
fix(api): resolve memory leak during user fetching

The previous implementation kept a DB connection open after every request. 
Switched to a scoped connection pool to ensure proper release.

Closes #142
```
