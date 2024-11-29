import numpy as np
import pandas as pd

dt = np.dtype(
    [
        ("scheme_state", "bool"),
        ("probability", "float64"),
        ("sv1", "bool", (8,)),
        ("sv2", "bool", (8,))
    ]
)

data = np.fromfile("simple-scored-state-set.sstd", dtype=dt)

data_flat = {
    "scheme_state": data["scheme_state"],
    "probability": data["probability"],
}

for i in range(8):
    data_flat[f"sv1_{i}"] = data["sv1"][:, i]
    data_flat[f"sv2_{i}"] = data["sv2"][:, i]

df = pd.DataFrame(data_flat)
print(df)
print(df.loc[df["scheme_state"] == True, "probability"].sum())
print(df.loc[df["scheme_state"] == False, "probability"].sum())