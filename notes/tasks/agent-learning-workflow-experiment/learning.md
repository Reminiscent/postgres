# Learning Closeout: agent-learning-workflow-experiment

Use this file when the task has a learning objective above `fix-only`, or when user intent clearly asks for deeper explanation, pattern audit, prevention guidance, or durable teaching material.
This file supplements `task.md` and `review.md`; it does not replace the normal evidence and readiness workflow.

## Trigger

- learning objective: teaching-pack
- why this closeout exists:
  - The user explicitly asked for first-principles design and best-practice workflow improvements rather than a narrow file edit.
- scope limits:
  - This closeout validates the experimental learning workflow on a workflow-design task.
  - It does not justify promoting the experiment into `AGENTS.md` yet.

## Context Pack

Use this for `deep-dive` and above.

- execution or call chain:
  - User request -> `agent-workflow-router` -> inspect proposal and sample materials in the sibling worktree -> inspect current hot-path docs and routing skill -> patch `README.agent.md`, `pg-task-intake`, and task templates -> self-review and closeout
- inputs and runtime state already prepared before the focal function:
  - The repo already had a clean control-plane split: `AGENTS.md` for durable hot-path rules, `README.agent.md` for workflow model, `pg-task-intake` for exact routing, and `notes/tasks/_template/` for durable memory.
- relevant planner / executor / JIT / memory-context or subsystem boundaries:
  - The workflow analogue here is control-plane ownership. Model statements belong in `README.agent.md`; exact routing and trigger rules belong in `.agents/skills/pg-task-intake/SKILL.md`; durable task artifacts belong in `notes/tasks/_template/`.
- why the logic belongs in this layer rather than a neighboring one:
  - Putting `Learning Objective` directly into `AGENTS.md` would prematurely harden an experiment into a hot-path rule.
  - Putting all trigger details into `README.agent.md` would duplicate the skill.
  - Putting the whole concept only in examples would leave the behavior too implicit to repeat reliably.

## Root-Cause Pack

Use this for `root-cause` and above.

- symptom or semantic mismatch:
  - The current workflow reliably standardizes correctness and reviewability, but learning-oriented outputs only appear after repeated follow-up prompts.
- broken assumption, hidden contract, or invalid abstraction:
  - The hidden assumption was that "good investigation notes plus good final explanations" are enough to surface deeper learning.
  - In practice, depth, sibling-path audit, and best-practice extraction are separate outputs with different stopping conditions and different user intent signals.
- why the behavior happened:
  - The existing workflow was optimized around proving correctness with minimal overhead. That made the hot path strong, but left deeper understanding as an ad hoc extension rather than a declared objective.
- why the chosen patch, conclusion, or guard works:
  - The patch adds an optional axis instead of widening the default one.
  - It preserves hot-path stability by keeping the experiment out of `AGENTS.md`, moves exact triggers into `pg-task-intake`, and gives deeper output a dedicated durable home in `learning.md`.
- what would regress if the broken assumption reappeared:
  - Agents would again need repeated user prompting to explain root cause, audit sibling paths, or distill best practices, and those outputs would remain inconsistent across tasks.

## Pattern Audit Pack

Use this for `pattern-audit` and above.
When the bug comes from semantic / optimized-path divergence, `NULL` or strictness semantics, a hidden fast-path contract, or a boundary-layer shortcut, sweep three to five sibling paths and classify each one.

| Path or construct | Why checked | Classification (`explicit guard` / `explicit non-strict support` / `contract assumption` / `unknown`) | Evidence | Risk or next step |
|---|---|---|---|---|
| `AGENTS.md` | Hot-path rule set must stay durable and light | `explicit guard` | `agent/EVOLUTION_RULES.md` and existing `AGENTS.md` already say not to grow rules without repeated failures | Keep deferred until the same friction recurs |
| `README.agent.md` | High-level workflow model is where optional layers should be named | `explicit guard` | Existing ownership rules already place model descriptions here | Safe to add a concise experimental section only |
| `.agents/skills/pg-task-intake/SKILL.md` | Exact trigger logic needs a single authoritative routing surface | `explicit guard` | The skill already owns classification, readiness, and read-first rules | Best place for `Learning Objective` inference and sibling-sweep routing |
| `notes/tasks/_template/` | Durable artifacts were missing a place for learning closeout | `explicit guard` | There was no learning template before; a dedicated `learning.md` now captures the output and gives the workflow an explicit durable sink | Validate on future tasks and adjust only if stable friction remains |
| separate repo-local teaching skill | Candidate future home if the workflow grows further | `contract assumption` | No stable input/output contract is proven yet; `EVOLUTION_RULES.md` requires recurrence before adding a skill | Defer until the experiment is reused enough to justify its own skill |

- generalized pattern:
  - When a workflow already proves correctness well, missing depth should be added as an optional axis with its own output contract rather than by making the default path heavier.
- search heuristic for future audits:
  - Look for recurring cases where users repeatedly ask "why", "what is the essence", "where else could this happen", or "how should we prevent this" after the core task is already effectively done.

## Refactor Pack

Optional. Use this when a cleanup, helper extraction, or patch-series discussion is a real next step.

- smallest refactor worth doing:
  - If the experiment proves useful across repeated tasks, the next refactor is to split the learning-oriented routing into its own narrow repo-local skill.
- why it is safe to defer or worth splitting:
  - The workflow contract is still being validated. Splitting now would create another abstraction before the stable boundaries are proven.
- alternatives that look cleaner but are not worth the scope:
  - Moving the whole experiment into `AGENTS.md`
  - Adding a teaching skill before repeated usage demonstrates clear independent value
  - Expanding every task template and review packet with mandatory learning fields

## Best Practices Extracted

Use this for `root-cause` and above.

- engineering principle violated:
  - A workflow should not rely on repeated user follow-up to surface a recurring class of high-value outputs.
- what to check first next time:
  - Whether the user is asking only for correctness, or also for root cause, system context, sibling risk, and prevention guidance.
- lowest-cost reliable prevention:
  - Add an optional intake field plus a dedicated closeout file before adding new permanent hot-path rules or skills.
- test or review heuristic worth reusing:
  - Dogfood new workflow templates on the task that introduces them; if the author cannot use the template naturally, the template is not ready.
- attractive but low-ROI alternative to avoid:
  - Converting every bugfix into a mandatory deep-dive. That increases prompt and review cost without respecting the user's actual objective.
