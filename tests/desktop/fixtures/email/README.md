# Synthetic legacy baseline fixtures

`synthetic-project.json` is deliberately fictional and contains the four workflow inputs used by the initial baseline capture. It covers managing/client/partner roles, special HTML characters, internal and external tracker records, multiple statuses/types, attendees, action items, recipient sources, reporting dates and status-report calculations. The broader matrix still needed for T00 is recorded in `docs/email-build/Progress.md`.

Run the capture without loading a plugin or Qt WebEngine:

```sh
python3 tests/desktop/fixtures/email/capture_legacy_baselines.py --output /tmp/projectnotes-email-baseline
```

The output directory contains four HTML documents and a JSON summary on stdout. Compare that summary with `baseline-summary.json` to detect legacy output drift. The tool parses the current legacy source and compiles only designated pure builders; it never invokes menu actions, collaboration adapters, settings, filesystem exports, processes, or network operations. Generated files are intentionally outside the repository.

The Meeting Notes Report fixture currently preserves the observed legacy behavior: the `internal_item` check decides inclusion, but the `includeitem` variable is not used to guard construction. This is a documented discrepancy, not an approved parity decision.
