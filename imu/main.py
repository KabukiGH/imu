import csv

# Path to the CSV file
csv_file_path = 'E:/Programs/C/Projects/Embevity/imu/mydata.csv'

# List to store data
file_data = []

# Reading the CSV file
try:
    with open(csv_file_path, newline='') as file:
        csv_reader = csv.reader(file)

        # Iterate through the first six rows
        for i, row in enumerate(csv_reader):
            if i < 6:
                file_data.append(row)
            else:
                break  # Stop after reading six rows
except FileNotFoundError:
    print(f"The file {csv_file_path} was not found.")
except Exception as e:
    print(f"An error occurred while reading the file: {e}")

# Displaying the data
for index, row in enumerate(file_data):
    print(f"Row {index + 1}: {row}")

# Example usage of the data
# Assuming each row is a list of values
# Adjust this part according to the structure of your CSV data
for row in file_data:
    # Perform operations on the read data here
    print(f"Processing row: {row}")