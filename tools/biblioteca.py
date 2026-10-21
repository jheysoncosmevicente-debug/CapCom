"""Genera los sonidos sinteticos de la Biblioteca CapCom (WAV 48 kHz, 24 bits).
Uso: uv run --with numpy --with scipy --with soundfile python tools/biblioteca.py "Biblioteca CapCom"

Los sonidos organicos (sirenas reales, lluvia, ciudad...) se descargan aparte; este script solo crea
lo que se puede sintetizar con precision: radio, tonos, cinta/TV, ambientes tecnicos y golpes de drama."""
import sys, os
import numpy as np
import soundfile as sf
from scipy import signal

SR = 48000
OUT = sys.argv[1]
rng = np.random.default_rng(7)


def t(seconds):
    return np.arange(int(seconds * SR)) / SR


def bandpass(x, lo, hi, order=4):
    sos = signal.butter(order, [lo, hi], btype="band", fs=SR, output="sos")
    return signal.sosfilt(sos, x)


def lowpass(x, f, order=4):
    return signal.sosfilt(signal.butter(order, f, btype="low", fs=SR, output="sos"), x)


def highpass(x, f, order=4):
    return signal.sosfilt(signal.butter(order, f, btype="high", fs=SR, output="sos"), x)


def fade(x, fin=0.003, fout=0.01):
    n_in, n_out = int(fin * SR), int(fout * SR)
    x = x.copy()
    if n_in: x[:n_in] *= np.linspace(0, 1, n_in)
    if n_out: x[-n_out:] *= np.linspace(1, 0, n_out)
    return x


def normalize(x, peak_db):
    p = np.max(np.abs(x)) + 1e-12
    return x / p * 10 ** (peak_db / 20)


def loopable(x, xfade=1.0):
    """Cruza el final con el principio para que el bucle no tenga salto."""
    n = int(xfade * SR)
    body, tail = x[:-n].copy(), x[-n:]
    ramp = np.linspace(0, 1, n)
    body[:n] = body[:n] * ramp + tail * (1 - ramp)
    return body


def radio_colour(x, drive=2.0):
    """Lo que hace una radio a un tono o ruido: banda 400-3000 Hz y algo de saturacion."""
    y = bandpass(x, 400, 3000)
    return np.tanh(y * drive) / np.tanh(drive)


def save(folder, name, x, stereo=False):
    path = os.path.join(OUT, folder)
    os.makedirs(path, exist_ok=True)
    if stereo and x.ndim == 1:
        x = np.stack([x, x], axis=1)
    sf.write(os.path.join(path, name + ".wav"), x.astype(np.float32), SR, subtype="PCM_24")
    print(f"{folder}/{name}.wav  {len(x) / SR:.1f} s")


def tone(freq, seconds, amp=1.0):
    return amp * np.sin(2 * np.pi * freq * t(seconds))


def sweep(f0, f1, seconds):
    tt = t(seconds)
    phase = 2 * np.pi * np.cumsum(np.linspace(f0, f1, len(tt))) / SR
    return np.sin(phase)


# ---------------------------------------------------------------- Radio
def kshh(length, hold, click=False, colour=0):
    n = int(length * SR)
    noise = rng.standard_normal(n)
    lo, hi = [(500, 3500), (700, 3000), (400, 4000)][colour]
    x = bandpass(noise, lo, hi)
    env = np.ones(n)
    h = int(hold * SR)
    env[h:] = np.linspace(1, 0, n - h) ** 2
    x = fade(x * env, 0.004, 0.005)
    if click:
        x[:int(0.002 * SR)] += 1.5 * np.max(np.abs(x))
    return normalize(x, -6)


for i in range(3):
    save("Radio", f"kshh_apertura_{i + 1:02d}", kshh(0.12 + 0.03 * i, 0.05, colour=i))
    save("Radio", f"kshh_cierre_{i + 1:02d}", kshh(0.30 + 0.06 * i, 0.06, click=True, colour=i))

ROGER = {
    "roger_bip_corto": [(0.0, 0.12, 1000, 1000)],
    "roger_doble_bip": [(0.0, 0.08, 1200, 1200), (0.14, 0.22, 1200, 1200)],
    "roger_motorola": [(0.0, 0.06, 1800, 1800), (0.06, 0.13, 1300, 1300)],
    "roger_nextel": [(0.0, 0.035, 1950, 1950), (0.05, 0.085, 2400, 2400)],
    "roger_militar_barrido": [(0.0, 0.15, 1800, 800)],
}
for name, segs in ROGER.items():
    x = np.zeros(int((segs[-1][1] + 0.02) * SR))
    for s0, s1, f0, f1 in segs:
        seg = fade(sweep(f0, f1, s1 - s0), 0.003, 0.003)
        a = int(s0 * SR)
        x[a:a + len(seg)] += seg
    save("Radio", name, normalize(radio_colour(x, 1.5), -6))

save("Radio", "quindar_inicio_nasa_2525hz", normalize(fade(tone(2525, 0.25), 0.004, 0.004), -6))
save("Radio", "quindar_fin_nasa_2475hz", normalize(fade(tone(2475, 0.25), 0.004, 0.004), -6))

# Estatica de radio en bucle: soplido de banda + crujidos
n = 32 * SR
static = bandpass(rng.standard_normal(n), 400, 3500) * 0.3
crackles = (rng.random(n) < 0.0008) * rng.standard_normal(n) * 3
static += bandpass(signal.lfilter([1], [1, -0.6], crackles), 300, 4000)
save("Radio", "estatica_radio_bucle_30s", normalize(loopable(static, 2.0), -12))

# Interferencia: silbido heterodino que se mueve + golpes de ruido
tt = t(8)
whistle = np.sin(2 * np.pi * np.cumsum(1200 + 900 * np.sin(2 * np.pi * 0.35 * tt)) / SR) * 0.25
bursts = bandpass(rng.standard_normal(len(tt)), 300, 4000) * (np.sin(2 * np.pi * 1.7 * tt) > 0.6) * 0.6
save("Radio", "interferencia_radio_8s", normalize(fade(radio_colour(whistle + bursts + 0.15 * rng.standard_normal(len(tt))), 0.05, 0.3), -8))

# Sintonizando AM/FM: barrido de silbidos y ruido, como mover el dial
tt = t(7)
dial = np.cumsum(rng.standard_normal(len(tt))) / SR * 40
carrier = np.sin(2 * np.pi * np.cumsum(500 + 2500 * np.abs(np.sin(2 * np.pi * 0.25 * tt + dial))) / SR)
noise = bandpass(rng.standard_normal(len(tt)), 300, 5000)
mixamt = 0.5 + 0.5 * np.sin(2 * np.pi * 0.6 * tt)
save("Radio", "radio_sintonizando_7s", normalize(fade(radio_colour(0.4 * carrier * mixamt + noise * (1 - 0.6 * mixamt), 1.2), 0.05, 0.3), -8))

# ---------------------------------------------------------------- Cinta / TV
n = 32 * SR
hiss = highpass(lowpass(rng.standard_normal(n), 9000), 1500, 2) * 0.5 + lowpass(rng.standard_normal(n), 120) * 0.15
save("Cinta y TV", "soplido_de_cinta_bucle_30s", normalize(loopable(hiss, 2.0), -18), stereo=True)

n = 10 * SR
snow = rng.standard_normal(n) * 0.5 + 0.12 * np.sign(np.sin(2 * np.pi * 60 * t(10)))
save("Cinta y TV", "tv_sin_senal_nieve_10s", normalize(fade(highpass(snow, 80), 0.05, 0.3), -10), stereo=True)
save("Cinta y TV", "tv_antigua_pitido_15khz_10s", normalize(fade(tone(15734, 10), 0.2, 0.2), -30))


def click(ms_len, bright, decay):
    n = int(ms_len / 1000 * SR)
    x = rng.standard_normal(n) * np.exp(-np.arange(n) / (decay * SR))
    return bandpass(x, 200, bright)


def place(total, events):
    out = np.zeros(int(total * SR))
    for at, x in events:
        a = int(at * SR)
        out[a:a + len(x)] += x[:len(out) - a]
    return out


play = place(0.9, [(0.0, click(40, 6000, 0.004)), (0.06, click(60, 3000, 0.01) * 0.6),
                   (0.12, lowpass(rng.standard_normal(int(0.7 * SR)), 300) * np.linspace(0, 0.25, int(0.7 * SR)))])
save("Cinta y TV", "casetera_play_clic", normalize(fade(play, 0.001, 0.1), -6))
stop = place(0.5, [(0.0, click(50, 5000, 0.006)), (0.05, click(40, 2500, 0.008) * 0.8)])
save("Cinta y TV", "casetera_stop_clic", normalize(fade(stop, 0.001, 0.05), -6))
crt_on = place(2.0, [(0.0, lowpass(click(120, 1500, 0.03), 400) * 2),
                     (0.05, highpass(sweep(9000, 15734, 1.9), 5000) * np.linspace(0.6, 0.15, int(1.9 * SR)))])
save("Cinta y TV", "tv_antigua_encendido", normalize(fade(crt_on, 0.001, 0.2), -6))

# ---------------------------------------------------------------- Ambientes tecnicos
tt = t(32)
hum = 0.5 * np.sin(2 * np.pi * 60 * tt) + 0.25 * np.sin(2 * np.pi * 120 * tt) + 0.15 * np.sin(2 * np.pi * 180 * tt)
buzz = bandpass(np.tanh(6 * np.sin(2 * np.pi * 60 * tt)), 1500, 3000, 2) * 0.12
save("Ambientes", "zumbido_electrico_60hz_bucle_30s", normalize(loopable(hum + buzz, 2.0), -18), stereo=True)

room = np.stack([lowpass(signal.lfilter([1], [1, -0.97], rng.standard_normal(32 * SR)), 800, 2) for _ in range(2)], axis=1)
room = np.stack([loopable(room[:, c], 2.0) for c in range(2)], axis=1)
save("Ambientes", "cuarto_vacio_room_tone_bucle_30s", normalize(room, -30))

# Sirena sintetica lejana (wail) con ecos, por si no hay una grabada que sirva
tt = t(32)
u = (tt % 4.6) / 4.6
freq = 650 + 750 * np.where(u < 0.6, np.sin(np.pi / 2 * u / 0.6), np.cos(np.pi / 2 * (u - 0.6) / 0.4))
ph = 2 * np.pi * np.cumsum(freq) / SR
siren = sum(np.sin(k * ph) / k for k in range(1, 6)) * (0.75 + 0.25 * np.sin(2 * np.pi * 0.11 * tt))
siren = lowpass(siren, 1800, 2)
left = siren + 0.45 * np.roll(siren, int(0.18 * SR)) + 0.3 * np.roll(siren, int(0.33 * SR))
right = siren + 0.45 * np.roll(siren, int(0.21 * SR)) + 0.3 * np.roll(siren, int(0.37 * SR))
save("Policia", "sirena_lejana_sintetica_bucle_30s", normalize(np.stack([loopable(left, 2.0), loopable(right, 2.0)], axis=1), -10))

# ---------------------------------------------------------------- Drama
def boom(seconds, f0, f1, punch, tail):
    tt = t(seconds)
    f = f1 + (f0 - f1) * np.exp(-tt / 0.08)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-tt / tail)
    hit = lowpass(rng.standard_normal(len(tt)), 2500) * np.exp(-tt / 0.015) * punch
    rumble = lowpass(rng.standard_normal(len(tt)), 150) * np.exp(-tt / (tail * 1.4)) * 0.6
    return fade(np.tanh(1.5 * (body + hit + rumble)), 0.001, 0.2)


save("Drama", "golpe_grave_01", normalize(boom(2.5, 90, 38, 0.8, 0.6), -1), stereo=True)
save("Drama", "golpe_grave_02_seco", normalize(boom(1.4, 110, 45, 1.2, 0.25), -1), stereo=True)
save("Drama", "golpe_grave_03_largo", normalize(boom(5.0, 70, 30, 0.5, 1.6), -1), stereo=True)

tt = t(3.5)
drop = np.sin(2 * np.pi * np.cumsum(30 + 90 * np.exp(-tt / 0.9)) / SR) * np.minimum(1, tt / 0.05) * np.exp(-tt / 1.6)
save("Drama", "sub_drop_3s", normalize(fade(drop, 0.001, 0.3), -1), stereo=True)

for secs in (5, 10):
    tt = t(secs)
    k = tt / secs
    noise = rng.standard_normal(len(tt))
    # Ruido que pasa de grave a agudo + tono que sube: tension que crece
    sweep_noise = (lowpass(noise, 800) * (1 - k) + highpass(noise, 3000) * k) * k ** 2
    rise = np.sin(2 * np.pi * np.cumsum(110 * 2 ** (2.5 * k)) / SR) * k ** 1.5 * 0.5
    x = fade(0.5 * sweep_noise + rise, 0.2, 0.02)
    save("Drama", f"subida_tension_{secs}s", normalize(x, -3), stereo=True)

tt = t(16)
beat = np.zeros(len(tt))
for start in np.arange(0, 16, 0.9):  # ~67 bpm
    for off, amp in ((0.0, 1.0), (0.22, 0.6)):  # lub-dub
        a = int((start + off) * SR)
        n = int(0.18 * SR)
        if a + n < len(beat):
            s = np.sin(2 * np.pi * 48 * np.arange(n) / SR) * np.exp(-np.arange(n) / (0.04 * SR)) * amp
            beat[a:a + n] += s
save("Drama", "latido_tension_67bpm_bucle_16s", normalize(lowpass(beat, 200), -6), stereo=True)


# ---------------------------------------------------------------- Alarma de auto
# Las alarmas reales son tonos electronicos por una bocina chica: ciclo de 4 patrones de ~2.5 s.
def car_alarm():
    seg = 2.5
    tt = t(seg)
    pats = []
    pats.append(600 + 900 * ((tt * 4) % 1))                                    # whoop subiendo, 4 por segundo
    pats.append(np.where((tt // 0.15) % 2 == 0, 800, 1200).astype(float))     # dos tonos alternados
    tri = 1 - np.abs(((tt % 1.0) / 1.0) * 2 - 1)
    pats.append(500 + 1300 * tri)                                             # barrido lento arriba/abajo
    pats.append(np.full_like(tt, 1300.0))                                     # chirrido entrecortado
    out = []
    for k, f in enumerate(pats):
        ph = 2 * np.pi * np.cumsum(f) / SR
        x = np.tanh(5 * np.sin(ph))                                           # onda casi cuadrada (bocina saturada)
        if k == 3:
            x *= ((tt * 12) % 1) < 0.5
        out.append(fade(x, 0.003, 0.01))
    x = np.concatenate(out * 2)                                               # dos vueltas: 20 s
    x = bandpass(x, 450, 4500, 2)
    horn = signal.tf2sos(*signal.iirpeak(2200, 3, fs=SR))   # resonancia de la bocina
    x = signal.sosfilt(horn, x) * 0.5 + x
    return x


alarm = car_alarm()
save("Policia", "alarma_de_auto_cerca_20s", normalize(fade(alarm, 0.005, 0.05), -3), stereo=True)
far = lowpass(alarm, 2500, 2)
far = far + 0.35 * np.roll(far, int(0.09 * SR)) + 0.2 * np.roll(far, int(0.21 * SR))  # rebote en edificios
save("Policia", "alarma_de_auto_lejana_20s", normalize(fade(far, 0.005, 0.3), -14), stereo=True)


# ---------------------------------------------------------------- Celular vibrando
# Motor de vibracion (~175 Hz) + golpeteo contra la mesa en cada vuelta del motor.
def phone_vibration(on_table=True):
    pattern = [(0.5, 1), (0.25, 0), (0.5, 1), (1.2, 0)] * 3
    total = sum(d for d, _ in pattern)
    env = np.concatenate([np.full(int(d * SR), v, float) for d, v in pattern])
    env = signal.lfilter([1 - 0.995], [1, -0.995], env)  # el motor tarda unos ms en arrancar y frenar
    tt = np.arange(len(env)) / SR
    f0 = 175 * (1 + 0.01 * np.sin(2 * np.pi * 3 * tt))
    ph = 2 * np.pi * np.cumsum(f0) / SR
    motor = np.sin(ph) + 0.5 * np.sin(2 * ph) + 0.3 * np.sin(3 * ph)
    if on_table:
        hits = (np.sin(ph) > 0.92) * rng.standard_normal(len(tt))            # el celular golpea la mesa
        rattle = bandpass(hits, 400, 6000, 2) * 2.5
        x = (0.35 * motor + rattle) * env
        x = x + 0.6 * lowpass(x, 300)                                         # la mesa resuena en graves
    else:
        x = lowpass(motor * env, 700, 4) * 1.2                                # dentro de un bolsillo o cartera
    return x, total


vib, secs = phone_vibration(True)
save("Foley", f"celular_vibrando_sobre_mesa_{int(round(secs))}s", normalize(fade(vib, 0.002, 0.05), -6), stereo=True)
vib, secs = phone_vibration(False)
save("Foley", f"celular_vibrando_en_bolsillo_{int(round(secs))}s", normalize(fade(vib, 0.002, 0.05), -10), stereo=True)
