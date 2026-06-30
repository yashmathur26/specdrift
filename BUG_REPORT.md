# SPECDRIFT Bug Report

Track known issues, reproduction steps, and fix status.

---

## Bug Template

```markdown
### [BUG-XXX] Brief Description
**Severity**: Critical / High / Medium / Low
**Component**: <which class/module>
**Discovered**: <date>
**Status**: Reported / In Progress / Fixed / Verified

**Description**:
<detailed explanation>

**Reproduction Steps**:
1. Step one
2. Step two
3. Expected vs Actual behavior

**Possible Causes**:
- Hypothesis 1

**Proposed Fixes**:
- Option 1: <description>

**Related Code**:
<file paths and line numbers>
```

---

## Active Bugs

### [BUG-001] FL Studio crash on plugin insert (macOS)
**Severity**: High
**Component**: PluginEditor, OrbKnobComponent
**Discovered**: 2025-02-24
**Status**: Mitigated

**Description**:
Access violation (EAccessViolation) when inserting Specdrift in FL Studio 2025 on macOS. Callstack involves libQuickFontCachev2_x64.dylib (FL Studio's font cache).

**Mitigation applied**:
- Removed custom font styles ("Medium", "Bold") that may trigger font loading
- Use FontOptions with height only (no style) for header/labels
- Orb knob value text uses Font(FontOptions(11.0f))
- Label components use default font (no setFont call)

**If crash persists**: Try AU format instead of VST3, or use Standalone app for testing.

---

## Resolved Bugs

*None yet.*
