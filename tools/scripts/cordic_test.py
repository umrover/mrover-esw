import argparse
import math
import random
import statistics
import struct
import sys
from dataclasses import dataclass

from esw import esw_logger
from esw.stlink.vcp import VCP, Message

# match classes from cordic.hpp
CONFIGS = {"cordic": 0, "software": 1}
KP = 0.5
SQRT3 = math.sqrt(3.0)
CORDIC_ERR = 2.0**-19
F32_EPS = 2.0**-23


@dataclass
class PipelineRequest(Message):
    FORMAT = "<Iffffff"
    config: int
    ia: float
    ib: float
    ic: float
    theta: float
    id_ref: float
    iq_ref: float


@dataclass
class PipelineResponse(Message):
    FORMAT = "<Ifffff"
    cycles: int
    id: float
    iq: float
    va: float
    vb: float
    vc: float


def clarke(a: float, b: float, c: float) -> tuple[float, float]:
    return (2.0 * a - b - c) / 3.0, (b - c) / SQRT3


def inv_clarke(alpha: float, beta: float) -> tuple[float, float, float]:
    return alpha, -0.5 * alpha + 0.5 * SQRT3 * beta, -0.5 * alpha - 0.5 * SQRT3 * beta


def park(alpha: float, beta: float, theta: float) -> tuple[float, float]:
    s, c = math.sin(theta), math.cos(theta)
    return alpha * c + beta * s, -alpha * s + beta * c


def inv_park(d: float, q: float, theta: float) -> tuple[float, float]:
    s, c = math.sin(theta), math.cos(theta)
    return d * c - q * s, d * s + q * c


def foc_step(ia: float, ib: float, ic: float, theta: float, id_ref: float, iq_ref: float) -> tuple[float, ...]:
    d, q = park(*clarke(ia, ib, ic), theta)
    return (d, q, *inv_clarke(*inv_park(KP * (id_ref - d), KP * (iq_ref - q), theta)))


def tolerance(ia: float, ib: float, ic: float, theta: float, id_ref: float, iq_ref: float) -> float:
    angle_err = 2.0 * F32_EPS * max(1.0, abs(theta))
    scale = max(1.0, abs(ia), abs(ib), abs(ic), abs(id_ref), abs(iq_ref))
    return scale * (1 + 2 * KP) * (4 * (CORDIC_ERR + angle_err) + 32 * F32_EPS)


def f32(x: float) -> float:
    return struct.unpack("<f", struct.pack("<f", x))[0]


def vectors(rng: random.Random, n: int, amp: float) -> list[tuple[float, ...]]:
    edges = [0.0, math.pi / 2, -math.pi / 2, math.pi, -math.pi, math.pi - 1e-6, 2 * math.pi, -2 * math.pi, 10.0, -10.0]
    angles = edges[:n] + [rng.uniform(-4 * math.pi, 4 * math.pi) for _ in range(max(0, n - len(edges)))]
    cases = []
    for theta in angles:
        m = rng.uniform(0, amp)
        i_abc = [m * math.cos(theta - k * 2 * math.pi / 3) + rng.uniform(-0.02 * amp, 0.02 * amp) for k in range(3)]
        cases.append(tuple(f32(x) for x in (*i_abc, theta, rng.uniform(-amp, amp), rng.uniform(-amp, amp))))
    return cases


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", "-p", help="serial port")
    parser.add_argument("--baud", "-b", type=int, default=115200, help="serial baud rate")
    parser.add_argument("--count", "-n", type=int, default=200, help="steps per config")
    parser.add_argument("--amplitude", "-a", type=float, default=100.0, help="max current")
    parser.add_argument("--seed", type=int, default=0, help="random seed")
    args = parser.parse_args()

    cases = vectors(random.Random(args.seed), args.count, args.amplitude)
    ok = True
    mean_cycles = {}

    with VCP(args.port, args.baud) as vcp:
        esw_logger.info(f"{'config':10s} {'n':>5s} {'max err':>11s} {'worst err':>9s} {'mean cycles':>12s}  result")
        for name, config in CONFIGS.items():
            cycles, max_err, worst, failures = [], 0.0, 0.0, []
            for case in cases:
                vcp.send(PipelineRequest(config, *case))
                resp = vcp.receive(PipelineResponse)
                if resp is None:
                    esw_logger.error(f"no response from the board (is fw flashed?) for {name} {case}")
                    return 1
                got = (resp.id, resp.iq, resp.va, resp.vb, resp.vc)
                want = foc_step(*case)
                err = max(abs(g - w) for g, w in zip(got, want, strict=True))
                ratio = err / tolerance(*case)
                cycles.append(resp.cycles)
                max_err, worst = max(max_err, err), max(worst, ratio)
                if ratio > 1:
                    failures.append((case, got, want))
            mean_cycles[name] = statistics.fmean(cycles)
            ok &= not failures
            verdict = "PASS" if not failures else f"FAIL ({len(failures)})"
            esw_logger.info(
                f"{name:10s} {len(cases):5d} {max_err:11.3e} {worst:9.3f} {mean_cycles[name]:12.1f}  {verdict}"
            )
            for case, got, want in failures[:3]:
                esw_logger.info(f"    in={case} board={got} python={want}")

    software, cordic = mean_cycles["software"], mean_cycles["cordic"]
    esw_logger.info(f"cordic is {software / cordic:.2f}x faster than software per FOC step")
    esw_logger.info("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
