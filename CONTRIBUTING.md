# Contributing to LoRaTNCX

Thanks for wanting to contribute. Seriously. You could be doing lots of other things (like watching cat videos),
but you're here. That means something. Here's how to make your contributions fast, pleasant, and actually useful.

## Quick contract
- Inputs: Issue reports, pull requests, patches, documentation updates, tests
- Outputs: Reviewed PRs, merged fixes, improved docs, working firmware on real hardware
- Error modes: Incomplete bug reports, failing hardware tests, poorly documented changes

## Filing an issue (do this first, please)
Good bug reports make lives better. Bad ones waste time.

When filing an issue, include:
- A short, descriptive title
- What you expected to happen vs what actually happened
- Steps to reproduce (exact commands, sample packets, or sequence of web UI actions)
- Hardware: board revision (V3, V4), antenna, power source
- Firmware: branch/commit (use `git rev-parse --short HEAD`) and build environment (PlatformIO, etc.)
- Logs: serial output, web server logs, or `dmesg` if relevant
- A minimal reproduction or test case if possible

If you come with only “it doesn't work”, expect a request for more details.

## Before opening a PR
- Search existing issues and PRs. Don't duplicate work unless you like doing that.
- Make sure the change is small and focused. One logical change per PR.
- Run the project's tests (if any) and include results in the PR description.
- Ensure code builds cleanly for both V3 and V4 targets when relevant.

## Code style and quality
- Keep code readable. Prefer clarity over cleverness.
- Follow the project's existing style. If in doubt, copy the surrounding code's style and commit message pattern.
- Comment non-obvious hardware quirks (pin differences between V3 and V4, inverted logic, etc.)
- Include unit tests where feasible. For embedded code, include host-side tests or a short hardware test procedure.

## Commit messages
- Use short, imperative summary lines (50 chars or less preferred), e.g. `radio: fix SF12 transmit timeout`
- If needed, include a longer description after a blank line describing motivation and side effects.
- Link issues in commit messages using `#<issue-number>` when relevant.

## Pull request checklist
- [ ] PR title is descriptive
- [ ] PR body explains why the change was made and how to test it
- [ ] Runs cleanly on PlatformIO (both `heltec_wifi_lora_32_V3` and `heltec_wifi_lora_32_V4` if affected)
- [ ] Relevant docs updated (README, docs/, or web UI files in `data/`)
- [ ] Tests added or existing tests updated

## Documentation updates
- If your change affects behavior, update `README.md` or the `docs/` files.
- If you add web UI elements, include both the UI assets in `data/` and any server-side changes.

## Labeling and issue workflow
- Use clear labels (bug, enhancement, docs, help wanted). If you can't, ask a maintainer.
- We triage regularly but we're not a huge team. Be patient; we value quality over speed.

## Maintainers & contact
- Primary maintainer: `@kc1awv` on GitHub. Mention them in an issue if you need attention.

If you'd like a private contact email added for sensitive reports, tell me the address and I will add it to `CODE_OF_CONDUCT.md` and here.

## Security issues
- For security-sensitive issues, open a private issue and mark it `SECURITY` in the title. We will coordinate a responsible disclosure and fix.

## Licensing and contributor rights
- By contributing, you agree to license your contribution under this repo's MIT license unless explicit arrangements are made.

## Final thoughts
Be bold. Break things in code, not in people. If your patch is honest and useful, we'll work with you to get it merged.

Thanks for helping make LoRaTNCX better — and funnier.
