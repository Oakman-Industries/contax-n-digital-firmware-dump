# Contributing

Thanks for helping preserve the Contax N Digital. This is a community effort, and contributions of all sizes are
welcome, from a single dump hash to a decoded settings map.

> **License note:** this project is **CC BY-NC-SA 4.0**. By contributing you agree your contribution is released under
> the same license, and that your own findings/derivatives stay **open** and **non-commercial** (see `LICENSE`).
> The only third-party files here are the 1.08 firmware files listed in `NOTICE` (© Kyocera Corporation, all rights
> retained). Please don't submit any other Kyocera/Contax firmware or software. Share dumps by **link** (see below),
> never as files committed here.

## Where help is most valuable

1. **Share a dump.** Use the **"Share a dump"** issue form. Include the SHA-256, the Step 5 check output, the firmware
   version before install, and a download link hosted wherever you like. Every body dumped helps show what is
   per-camera, what is common, and whether other firmware variants ever existed.
2. **Report your install, good or bad.** Use the **"Install report"** form. A failed install is just as important to
   record as a success. It's the only way the real-world risk becomes known.
3. **Document what the camera shows.** The on-screen messages during install, during a dump, and when there's no
   marker file aren't documented yet. Photos of the screen are very welcome.
4. **Decode the dump.** Map individual settings (one-setting-changed dump pairs, see `docs/DUMP_FORMAT.md`), explain
   the host-side CRC mismatch, or work out whether the live-state area is flash or battery-backed memory.

## How to contribute

1. Fork the repo and create a branch.
2. Make your change. For the firmware, keep it small, read-only and fail-closed. Any change to the dump code needs
   its own careful review, because a flashed mistake can brick a camera.
3. If you change `firmware/src/`, rebuild with `firmware/src/build.sh`. Update `firmware/SHA256SUMS`, the IPS patch, the
   ready-made `cf-card/` image and the hashes in `README.md` together, and say clearly in the PR that the new build is **untested on hardware** until
   someone has flashed it.
4. Keep `README.md` and `docs/DUMP_FORMAT.md` in step with any behaviour change.
5. Open a pull request describing **what** you changed, **why**, and **how you validated it**. In keeping with the
   license, please share the findings behind your change openly, not just the code.

## Reporting issues

Open an issue with: your OS, the exact commands you ran, the full output/error, the hashes you got, and what the camera
showed. Don't attach dumps or any Kyocera/Contax files to issues or PRs. Link to them instead. Leave out personal
details you don't want to publish (serial numbers are optional).

## Code of conduct

Be kind and constructive. This is a small preservation project run by volunteers for owners of a long-discontinued
camera; assume good faith.

## Support

If this helped you, support ongoing Contax N Digital work at **[ko-fi.com/oakman](https://ko-fi.com/oakman)**. It
funds hardware, test cameras, media, and time for further research.
