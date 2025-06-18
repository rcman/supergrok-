Here is an optimized version of the stock trading simulation code. I've improved efficiency by:

Fetching only necessary data from yfinance
Using vectorized operations for signal generation
Reducing iterations in trading simulation to only trade dates
Maintaining accuracy with proper crossover detection
stock_trading_simulation.py
python
Show inline
This version is more efficient because:

It minimizes data retrieval by fetching only 'Adj Close'
Uses vectorized pandas operations for moving averages and signals
Reduces trading simulation iterations to only trade dates
Pre-allocates the portfolio array with NumPy for faster computation
Maintains clean, modular code structure for readability and maintenance
