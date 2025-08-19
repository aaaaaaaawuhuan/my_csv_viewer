import csv
import random
from datetime import datetime, timedelta

def generate_test_csv(filename, rows=1100):
    # 定义一些示例数据
    first_names = ['John', 'Jane', 'Bob', 'Alice', 'Charlie', 'Diana', 'Edward', 'Fiona', 'George', 'Helen']
    last_names = ['Smith', 'Johnson', 'Williams', 'Brown', 'Jones', 'Miller', 'Davis', 'Garcia', 'Rodriguez', 'Wilson']
    departments = ['HR', 'Engineering', 'Marketing', 'Sales', 'Finance', 'Operations', 'IT', 'Legal']
    cities = ['New York', 'Los Angeles', 'Chicago', 'Houston', 'Phoenix', 'Philadelphia', 'San Antonio', 'San Diego']
    
    with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = ['ID', 'First Name', 'Last Name', 'Email', 'Department', 'Salary', 'Join Date', 'City']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        # 写入表头
        writer.writeheader()
        
        # 生成指定行数的数据
        for i in range(1, rows + 1):
            first_name = random.choice(first_names)
            last_name = random.choice(last_names)
            email = f"{first_name.lower()}.{last_name.lower()}{i}@company.com"
            department = random.choice(departments)
            salary = random.randint(30000, 150000)
            
            # 生成随机日期
            start_date = datetime(2020, 1, 1)
            end_date = datetime(2024, 12, 31)
            random_date = start_date + timedelta(days=random.randint(0, (end_date - start_date).days))
            join_date = random_date.strftime('%Y-%m-%d')
            
            city = random.choice(cities)
            
            writer.writerow({
                'ID': i,
                'First Name': first_name,
                'Last Name': last_name,
                'Email': email,
                'Department': department,
                'Salary': salary,
                'Join Date': join_date,
                'City': city
            })
    
    print(f"已生成包含 {rows} 行数据的CSV文件: {filename}")

if __name__ == "__main__":
    generate_test_csv("test_data.csv", 1100)