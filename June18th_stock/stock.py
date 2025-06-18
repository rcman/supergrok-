import pandas as pd
import yfinance as yf
import numpy as np
import matplotlib.pyplot as plt

def fetch_stock_data(ticker, start_date, end_date):
    """Fetch adjusted close prices for a given ticker."""
    stock = yf.download(ticker, start=start_date, end=end_date, progress=False)
    return stock['Adj Close']

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
    cash = initial_cash
    holdings = 0
    portfolio = np.zeros(len(signals))
    
    # Iterate only over trade dates for efficiency
    for i, date in enumerate(signals.index):
        if date in trade_dates:
            pos = signals.loc[date, 'positions']
            price = signals.loc[date, 'price']
            if pos == 1 and cash > 0:
                shares = cash // price
                holdings += shares
                cash -= shares * price
            elif pos == -1 and holdings > 0:
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
    plt.plot(buy_dates, signals.loc[buy_dates, 'price'], '^', markersize=10, color='g', label='Buy')
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
    
    # Execute simulation
    prices = fetch_stock_data(ticker, start_date, end_date)
    short_mavg, long_mavg = calculate_moving_averages(prices, short_window, long_window)
    signals = generate_signals(prices, short_mavg, long_mavg)
    signals, final_value = simulate_trading(signals, initial_cash)
    plot_results(signals, ticker)
    
    # Display results
    print(f"Final Portfolio Value: ${final_value:.2f}")
    print(f"Profit/Loss: ${(final_value - initial_cash):.2f}")
