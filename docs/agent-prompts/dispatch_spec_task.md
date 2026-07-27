# Specification Task Dispatcher

You are executing an implementation specification stored inside this repository.

The specification is:

```text
docs/specifications/telemetry/telemetry_spec.md
```

Your job is to execute exactly one task from that specification.

You are not authorised to complete the full specification in one run.

---

## Required Process

1. Open the specification.

2. Find the heading:

```text
# 9. Task Index
```

3. Read the task table.

4. Select the first task marked `TODO` whose preceding dependencies are complete.

5. Read the complete task definition under:

```text
# 10. Task Definitions
```

6. Locate the heading matching the selected task ID.

Example:

```text
## TEL-001 — Audit the Legacy Telemetry Implementation
```

7. Before editing production code, update that task in the Task Index from:

```text
TODO
```

to:

```text
IN_PROGRESS
```

8. Save the specification file.

9. Execute only the selected task.

10. Follow all restrictions, completion criteria, build rules and size limits in the specification.

11. Do not begin any other task.

12. When the selected task is complete:

- update its status to `COMPLETE`
- save the specification
- produce the required end-of-run report
- stop

13. When genuinely blocked:

- update its status to `BLOCKED`
- save the specification
- explain the exact blocker
- stop

---

## Persistent State

The Task Index inside the specification is the persistent task memory.

Do not invent a separate task list.

Do not rely on conversation memory for task status.

Always re-read the Task Index at the start of a new run.

The specification file is the source of truth.

---

## Reading Rules

Read files in meaningful sections.

Do not repeatedly read the same lines.

Never request the same exact file range more than three times without a file change.

When reading a large source file:

1. determine its total size if possible
2. read sequential non-overlapping sections
3. record which sections have already been read
4. do not restart from the beginning after compaction
5. use headings, symbols or function names instead of repeatedly requesting arbitrary tail lines

For the legacy file:

```text
src/telemetry.cpp
```

read sequential sections and maintain an inventory of completed sections.

Do not repeatedly request the final lines of the file.

---

## Compaction Recovery

After conversation compaction:

1. re-open the specification
2. read the Task Index
3. identify the single `IN_PROGRESS` task
4. re-read only that task definition
5. inspect the current repository changes
6. continue that task only

Do not restart the entire specification.

Do not select a new task while another task is `IN_PROGRESS`.

When no task is `IN_PROGRESS`, select the next eligible `TODO` task.

---

## Implementation Limits

These limits are mandatory:

```text
.cpp file: maximum 300 lines
.h file: maximum 200 lines
function: maximum 50 lines
one cohesive responsibility per class
one task per run
```

When a limit would be exceeded, split the responsibility within the current task only when the specification allows it.

Do not create oversized files merely to finish quickly.

---

## Behaviour Rules

- Do not invent requirements.
- Do not silently alter existing behaviour.
- Do not rewrite unrelated files.
- Do not implement future tasks early.
- Do not delete legacy code until the specified migration task permits it.
- Do not claim a build passed without running it.
- Do not claim tests passed without running them.
- Do not continue automatically after completing the selected task.

---

## Loop Protection

Stop and report a blocker when:

- the same tool call fails three times
- the same file range is read three times without new information
- the same build error remains after three focused fixes
- required information is absent from the repository and specification
- conversation compaction loses necessary implementation state
- the selected task cannot be completed without performing another task
- repository contents contradict the specification materially

Do not continue an unproductive loop.

---

## Required Final Output

At the end of the run output exactly:

```text
Specification:
Current task:
Task status:
Files read:
Files created:
Files modified:
Build command:
Build result:
Tests run:
Test result:
Size-limit check:
Blockers:
Next task:
```

Then stop.