#!/usr/bin/python

from PIL import Image
import numpy as np

def pack_pbr_textures(metalness_path, roughness_path, ao_path, output_path):
    # Load the textures
    metalness = Image.open(metalness_path).convert("L")  # Convert to grayscale
    roughness = Image.open(roughness_path).convert("L")
    ao = Image.open(ao_path).convert("L")

    # Convert to numpy arrays for manipulation
    metalness_data = np.array(metalness)
    roughness_data = np.array(roughness)
    ao_data = np.array(ao)

    # Ensure all arrays are the same size
    if metalness_data.shape != roughness_data.shape or metalness_data.shape != ao_data.shape:
        raise ValueError("All input textures must have the same dimensions")

    # Stack the textures into an RGB image
    packed_data = np.stack([metalness_data, roughness_data, ao_data], axis=-1)

    # Create a new Image from the packed data
    packed_image = Image.fromarray(packed_data.astype(np.uint8))

    # Save the packed texture
    packed_image.save(output_path)
    print(f"Packed texture saved to: {output_path}")


metalness_path = "Ground068_2K-PNG_Metalness.png"
roughness_path = "Ground068_2K-PNG_Roughness.png"
ao_path = "Ground068_2K-PNG_AmbientOcclusion.png"
output_path = "Ground068_2K-PNG_Packed.png"
pack_pbr_textures(metalness_path, roughness_path, ao_path, output_path)
