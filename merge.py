import gpxpy
import gpxpy.gpx


def merge_gpx_files(input_files, output_file):
    # Create a new GPX object to hold the merged data
    merged_gpx = gpxpy.gpx.GPX()

    for file in input_files:
        # Parse the GPX file
        with open(file, 'r') as gpx_file:
            gpx = gpxpy.parse(gpx_file)

            # Add tracks from the current file to the merged GPX
            for track in gpx.tracks:
                merged_gpx.tracks.append(track)

            # Add routes from the current file to the merged GPX
            for route in gpx.routes:
                merged_gpx.routes.append(route)

            # Add waypoints from the current file to the merged GPX
            for waypoint in gpx.waypoints:
                merged_gpx.waypoints.append(waypoint)

    # Write the merged GPX to the output file
    with open(output_file, 'w') as f:
        f.write(merged_gpx.to_xml())


# Example usage
input_files = [
    "gpx/2024 Sommertur/Connected_20240625_102547_Litldalsvegen_6600_Sunndalsøra.gpx",
    "gpx/2024 Sommertur/Connected_20240625_150703_Svartvassvegen_1_6650_Surnadal.gpx",
    "gpx/2024 Sommertur/Connected_20240625_181119_Svartvassvegen_1_6650_Surnadal.gpx"
]
output_file = "gpx/2024 Sommertur/days/Connected_20240625_merged.gpx"
merge_gpx_files(input_files, output_file)
