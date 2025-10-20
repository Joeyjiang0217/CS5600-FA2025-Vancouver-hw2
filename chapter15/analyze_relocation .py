import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np

def run_relocation(seed, limit, num_addresses=10):
    """
    Run the relocation.py program and count valid addresses.
    
    Returns:
        tuple: (valid_count, total_count)
    """
    cmd = [
        'python3', './relocation.py',
        '-s', str(seed),
        '-n', str(num_addresses),
        '-l', str(limit),
        '-c'
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
        output = result.stdout
        
        # Count VALID and SEGMENTATION VIOLATION occurrences
        valid_count = len(re.findall(r'VALID:', output))
        total_count = len(re.findall(r'VA\s+\d+:', output))
        
        return valid_count, total_count
    except Exception as e:
        print(f"Error running simulation: {e}")
        return 0, 0

def main():
    # Parameters
    num_trials = 100
    num_addresses = 10
    address_space_size = 1024  # 1k default
    
    # Range of limit values from 0 to max address space size
    limit_values = range(0, address_space_size + 1, address_space_size // 50)  # 51 points
    
    # Store results
    results = {limit: [] for limit in limit_values}
    
    print("Running simulations...")
    print(f"Testing {len(limit_values)} different limit values")
    print(f"Running {num_trials} trials per limit value")
    print(f"Total simulations: {len(limit_values) * num_trials}")
    
    # Run simulations
    for limit in limit_values:
        print(f"\nTesting limit = {limit}")
        valid_fractions = []
        
        for seed in range(num_trials):
            valid_count, total_count = run_relocation(seed, limit, num_addresses)
            
            if total_count > 0:
                fraction = valid_count / total_count
                valid_fractions.append(fraction)
            
            # Progress indicator
            if (seed + 1) % 100 == 0:
                print(f"  Completed {seed + 1}/{num_trials} trials")
        
        results[limit] = valid_fractions
    
    # Calculate statistics for each limit value
    limit_list = []
    mean_fractions = []
    std_fractions = []
    
    for limit in sorted(results.keys()):
        if results[limit]:
            limit_list.append(limit)
            mean_fractions.append(np.mean(results[limit]))
            std_fractions.append(np.std(results[limit]))
    
    # Create the plot
    plt.figure(figsize=(12, 8))
    
    # Plot mean without error bars
    plt.plot(limit_list, mean_fractions, 
             'o-', linewidth=2, markersize=6, alpha=0.7, 
             label='Mean Fraction')
    
    # Add a reference line showing the theoretical expectation
    theoretical = [limit / address_space_size for limit in limit_list]
    plt.plot(limit_list, theoretical, 'r--', linewidth=2, 
             label='Theoretical (limit/address_space_size)', alpha=0.7)
    
    plt.xlabel('Limit Register Value', fontsize=12)
    plt.ylabel('Fraction of Valid Addresses', fontsize=12)
    plt.title(f'Fraction of Valid Virtual Addresses vs Limit Register Value\n' + 
              f'({num_trials} trials per limit, {num_addresses} addresses per trial)', 
              fontsize=14)
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)
    plt.xlim(0, address_space_size)
    plt.ylim(0, 1.05)
    
    # Save the figure
    plt.tight_layout()
    plt.savefig('valid_addresses_vs_limit.png', dpi=300, bbox_inches='tight')
    print("\n\nGraph saved as 'valid_addresses_vs_limit.png'")
    
    # Show the plot
    plt.show()
    
    # Print summary statistics
    print("\n" + "="*60)
    print("SUMMARY STATISTICS")
    print("="*60)
    print(f"{'Limit':<10} {'Mean Fraction':<15} {'Std Dev':<15} {'Theoretical':<15}")
    print("-"*60)
    for i, limit in enumerate(limit_list):
        print(f"{limit:<10} {mean_fractions[i]:<15.4f} {std_fractions[i]:<15.4f} {theoretical[i]:<15.4f}")

if __name__ == "__main__":
    main()