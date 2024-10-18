from __future__ import annotations
import fastgpx
__all__ = ['decode', 'encode']
def decode(encoded: str) -> list[fastgpx.LatLong]:
    ...
def encode(locations: list[fastgpx.LatLong]) -> str:
    ...
