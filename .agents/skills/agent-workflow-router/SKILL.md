---
name: agent-workflow-router
description: Use this skill when the task is about improving, reviewing, debugging, or evaluating the local agent workflow itself rather than changing PostgreSQL code or PostgreSQL behavior. Typical targets include AGENTS.md, README.agent.md, repo-local skills, note templates, cold-path workflow assets, and workflow structure.
---

# agent-workflow-router

## Purpose

Choose the right **work mode** for agent-workflow tasks.

Use this skill when the primary object is the local workflow itself: rules, skills, task-memory templates, directory layout, or agent failure modes.

Do **not** use this skill for normal PostgreSQL engineering work. If the task is mainly about PostgreSQL code, semantics, tests, or performance evidence, use `pg-task-intake` instead.

If the current runtime does not expose repo-local skills directly, follow this file as an inline playbook and perform the same routing steps manually.

## Inputs

- user request
- target files or directories
- whether the task changes workflow behavior
- whether the task investigates a workflow failure
- whether the task compares workflow variants or measures quality
- whether the task is mainly review/advice rather than implementation

## Work modes

### 1. workflow-change
Use when the task changes how the local agent workflow behaves or is organized.

Typical examples:
- editing `AGENTS.md`
- adding or updating a repo-local skill
- changing `README.agent.md`
- changing note templates
- moving hot-path or cold-path assets

Output requirements:
- problem statement,
- current behavior,
- desired behavior,
- smallest coherent change,
- affected operator-facing docs,
- explicit note of what remains deferred.

### 2. failure-analysis
Use when the task is to explain or fix why the workflow produced a poor result.

Typical examples:
- wrong skill triggered,
- missing rationale caused a bad conclusion,
- the workflow produced too much noise,
- a required check was skipped,
- a reviewer packet was incomplete or misleading.

Output requirements:
- concrete failure description,
- likely failure surface,
- evidence or reproduction artifact,
- smallest fix hypothesis,
- regression check to prevent recurrence.

### 3. evaluation
Use when the task is to measure whether a workflow change improves output quality.

Typical examples:
- compare old vs. new instructions,
- score rationale quality,
- measure reviewer-packet completeness,
- assess routing precision/recall,
- evaluate whether a new skill is helping.

Output requirements:
- evaluation question,
- task set or examples,
- metric or scorecard,
- comparison method,
- caveats and confounders,
- decision rule for keeping or rejecting the change.

### 4. review
Use when the task is to inspect the current workflow or a proposed change and produce findings.

Typical examples:
- what should stay or be removed,
- whether a new skill is justified,
- whether a document is on the right path,
- whether a layout or rule is over-complicated.

Output requirements:
- findings ordered by importance,
- risks,
- open questions,
- recommended next step.

## Routing rules

- If the task is mainly about `AGENTS.md`, `README.agent.md`, `.agents/skills/`, `agent/`, `notes/tasks/_template/`, or workflow structure, use this skill.
- If the main work is changing those assets, choose **workflow-change**.
- If the main work is explaining a poor agent result or preventing repeat workflow mistakes, choose **failure-analysis**.
- If the main work is measuring whether a workflow variant is better, choose **evaluation**.
- If the main work is inspecting existing workflow assets and recommending what to keep/change/remove, choose **review**.
- If the main work is PostgreSQL engineering rather than workflow evolution, do not use this skill.

## Required follow-up work by mode

- **workflow-change**: inspect all affected operator-facing files, keep the patch minimal, and record hot-path/cold-path impact
- **failure-analysis**: capture the failure case, identify the failing layer, propose the smallest fix, and define a regression check
- **evaluation**: define the task set, metrics, comparison method, and acceptance threshold; add minimal evaluation assets only if the task warrants them
- **review**: inspect the relevant assets, produce findings, and decide whether action is needed now or can be deferred

## Output format

Return:

1. chosen work mode,
2. why that mode was chosen,
3. primary assets in scope,
4. evidence required,
5. what is safe to defer,
6. recommended next step.
