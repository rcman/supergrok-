import pandas as pd
import yfinance as yf
import numpy as np
import matplotlib.pyplot as plt
import time

def fetch_stock_data(ticker, start_date, end_date):
    """Fetch adjusted close prices for a given ticker."""
    try:
        stock = yf.download(ticker, start=start_date, end=end_date, progress=False, auto_adjust=False)
        if stock.empty:
            raise ValueError("No data fetched for the given date range.")
        if 'Adj Close' not in stock.columns:
            raise KeyError("Expected 'Adj Close' column not found in data.")
        return stock['Adj Close']
    except Exception as e:
        print(f"Error fetching data for {ticker}: {e}")
        return None

def calculate_moving_averages(prices, short_window, long_window):
    """Calculate short and long moving averages."""
    short_mavg = prices.rolling(window=short_window).mean()
    long_mavg = prices.rolling(window=long_window).mean()
    return short_mavg, long_mavg

def generate_signals(prices, short_mavg, long_mavg):
    """Generate buy/sell signals based on moving average crossovers."""
    signals = pd.DataFrame(index=prices.index)
    signals['price'] = prices
    signals['short_mavg'] = short_mavg
    signals['long_mavg'] = long_mavg
    signals['buy_signal'] = ((short_mavg > long_mavg) & (short_mavg.shift(1) <= long_mavg.shift(1))).astype(int)
    signals['sell_signal'] = ((short_mavg < long_mavg) & (short_mavg.shift(1) >= long_mavg.shift(1))).astype(int)
    signals['positions'] = signals['buy_signal'] - signals['sell_signal']
    return signals

def simulate_trading(signals, initial_cash):
    """Simulate trading and calculate portfolio value over time."""
    trade_dates = signals.index[signals['positions'] != 0]
    if trade_dates.empty:
        print("No trade signals generated. Check moving average windows or data range.")
        return signals, initial_cash
    
    cash = initial_cash
    holdings = 0
    portfolio = np.zeros(len(signals))
    
    for i, date in enumerate(signals.index):
        if date in trade_dates:
            pos = signals.loc[date, 'positions']
            price = signals.loc[date, 'price']
            if pos == 1 and cash > 0:  # Buy
                shares = cash // price
                holdings += shares
                cash -= shares * price
            elif pos == -1 and holdings > 0:  # Sell
                cash += holdings * price
                holdings = 0
        portfolio[i] = cash + holdings * signals['price'].iloc[i]
    
    signals['portfolio'] = portfolio
    return signals, portfolio[-1]

def plot_results(signals, ticker):
    """Plot price, moving averages, and trading signals."""
    plt.figure(figsize=(12, 6))
    plt.plot(signals['price'], label='Price')
    plt.plot(signals['short_mavg'], label='Short MA')
    plt.plot(signals['long_mavg'], label='Long MA')
    buy_dates = signals.index[signals['positions'] == 1]
    sell_dates = signals.index[signals['positions'] == -1]
    if not buy_dates.empty:
        plt.plot(buy_dates, signals.loc[buy_dates, 'price'], '^', markersize=10, color='g', label='Buy')
    if not sell_dates.empty:
        plt.plot(sell_dates, signals.loc[sell_dates, 'price'], 'v', markersize=10, color='r', label='Sell')
    plt.title(f'{ticker} Trading Strategy')
    plt.legend()
    plt.show()

if __name__ == "__main__":
    # Parameters
    ticker = "AAPL"
    start_date = "2023-01-01"
    end_date = "2025-06-18"
    initial_cash = 10000
    short_window = 20
    long_window = 50
    
    # Fetch data with retry for rate limit errors only
    prices = fetch_stock_data(ticker, start_date, end_date)
    if prices is None:
        print("Possible rate limit detected. Retrying after 10 seconds...")
        time.sleep(10)
        prices = fetch_stock_data(ticker, start_date, end_date)
    if prices is None:
        print("Failed to fetch data after retry. Please check your internet connection, yfinance version, or try again later.")
        exit()
    
    # Debug: Print available columns
    stock = yf.download(ticker, start=start_date, end=end_date, progress=False, auto_adjust=False)
    print(f"Available columns in data: {stock.columns.tolist()}")
    
    # Check data sufficiency
    if len(prices) < long_window:
        print(f"Insufficient data: only {len(prices)} days available, need at least {long_window} for moving averages.")
        exit()
    
    # Calculate moving averages and generate signals
    short_mavg, long_mavg = calculate_moving_averages(prices, short_window, long_window)
    signals = generate_signals(prices, short_mavg, long_mavg)
    
    # Simulate trading
    signals, final_value = simulate_trading(signals, initial_cash)
    
    # Plot and display results
    plot_results(signals, ticker)
    print(f"Final Portfolio Value: ${final_value:.2f}")
    print(f"Profit/Loss: ${(final_value - initial_cash):.2f}")
