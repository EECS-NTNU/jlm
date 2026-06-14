---
name: writing-cline-skills
description: Help users create and write their own Cline skills. Use when developing, building, or documenting new Cline skills. Covers YAML frontmatter, SKILL.md structure, naming conventions, and best practices for skill development.
---

# Writing Cline Skills

This skill helps you create and write your own Cline skills. Follow these guidelines to build effective, reusable instruction sets that extend Cline's capabilities.

## Skill Structure
Create the skill directory with a helper script:
```bash
#!/usr/bin/env bash
set -e
skill_name=$1
mkdir -p ".cline/skills/$skill_name"
cat > ".cline/skills/$skill_name/SKILL.md" <<'EOF'
---
name: $skill_name
description: Brief description of what this skill does and when to use it.
---
# $skill_name Skill

## Overview

<!-- Add detailed description here -->
EOF
echo "Skill scaffold created at .cline/skills/$skill_name"
```
Make the script executable (`chmod +x scripts/create-skill.sh`) and run `./scripts/create-skill.sh my-new-skill`.

-------

Every Cline skill is a directory containing:

```
my-skill/
├── SKILL.md          # Required: main instructions
├── docs/             # Optional: additional documentation
│   └── advanced.md
└── scripts/          # Optional: utility scripts
    └── helper.sh
```

## YAML Frontmatter

Every `SKILL.md` file must start with YAML frontmatter containing two required fields:

```markdown
---
name: my-skill
description: Brief description of what this skill does and when to use it.
---
```

**Required fields:**
- `name`: Must exactly match the directory name (use kebab-case)
- `description`: Tells Cline when to activate the skill (max 1024 characters)

## Naming Conventions

**Good names (kebab-case, descriptive):**
- `aws-cdk-deploy`
- `pr-review-checklist`
- `database-migration`
- `api-client-generator`

**Avoid:**
- `aws` (too vague)
- `my_skill` (underscores, not descriptive)
- `DeployToAWS` (PascalCase, not kebab-case)
- `misc-helpers` (too generic)

## Writing Effective Descriptions

The description determines when Cline activates your skill. A vague description means the skill won't trigger when expected.

**Good descriptions are specific and actionable:**
- "Deploy applications to AWS using CDK. Use when deploying, updating infrastructure, or managing AWS resources."
- "Generate release notes from git commits. Use when preparing releases, writing changelogs, or summarizing recent changes."
- "Analyze CSV and Excel data files. Use when exploring datasets, generating statistics, or creating visualizations from tabular data."

**Weak descriptions leave too much ambiguity:**
- "Helps with AWS stuff."
- "Data analysis helper."
- "Useful for releases."

## Best Practices
- Keep `SKILL.md` under 5 k tokens; split large content into a `docs/` subdirectory.
- Include real command examples and expected output.
- Front‑load the most common usage patterns so Cline can scan them quickly.

-------

1. **Keep it focused**: Keep `SKILL.md` under 5k tokens. If content grows, split into `docs/` subdirectories.

2. **Include real examples**: Show actual commands, expected output, and results. Abstract instructions are harder to follow.

3. **Use clear section headers**: Headers like `## Error Handling` or `## Configuration` help Cline scan for relevant sections.

4. **Front-load important info**: Cline reads sequentially, so put common cases first.

## Bundling Supporting Files

You can include additional files in subdirectories:

### `docs/` - Additional Documentation
Use for detailed or situational content:
- Advanced configuration options
- Troubleshooting guides for edge cases
- Reference material (API schemas, database schemas)
- Platform-specific instructions

### `templates/` - Template Files
Use when creating configuration files or boilerplate:
- Config files (Terraform, Docker Compose)
- Code scaffolding (component templates, test fixtures)
- Documentation templates (README, API docs)

### `scripts/` - Utility Scripts
Use for deterministic operations:
- Validation (linting configs, checking prerequisites)
- Data processing (parsing, formatting)
- Complex calculations (cost estimation)
- API interactions (fetching data, health checks)

Scripts are token-efficient because only their output enters context, not the code itself.

## Referencing Bundled Files

In your `SKILL.md`, reference supporting files using markdown links:

```markdown
For initial setup, follow [setup.md](docs/setup.md).

Use the config template at `templates/config.yaml` as a starting point.

Run the validation script: python scripts/validate.py
```

Cline reads documentation files using `read_file` when referenced. Scripts are executed directly.

## Creating Your Skill

1. **Create the directory**: Place in `.cline/skills/` (workspace) or `~/.cline/skills/` (global)
2. **Add SKILL.md**: Follow the structure above with YAML frontmatter
3. **Test your description**: Try different phrasings to ensure the skill triggers correctly
4. **Iterate**: Refine the description based on when it does/doesn't trigger

## Skill Activation

Skills use progressive loading:
- **Metadata**: Always loaded (~100 tokens per skill) - name and description
- **Instructions**: Loaded when skill is triggered (under 5k tokens)
- **Resources**: Loaded as needed (effectively unlimited)

When you send a message, Cline sees a list of available skills with their descriptions. If your request matches a skill's description, Cline activates it using the `use_skill` tool.