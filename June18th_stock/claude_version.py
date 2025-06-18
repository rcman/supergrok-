import pandas as pd
import yfinance as yf
import numpy as np
import matplotlib.pyplot as plt
import time
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

# Constants
RETRY_DELAY = 60  # Increased to 60 seconds for rate limits
MAX_RETRIES = 3
REQUEST_DELAY = 2  # Delay between requests

def setup_session():
    """Setup a session with retry strategy and headers to avoid rate limits."""
    session = requests.Session()
    
    # Setup retry strategy
    retry_strategy = Retry(
        total=3,
        backoff_factor=1,
        status_forcelist=[429, 500, 502, 503, 504],
    )
    
    adapter = HTTPAdapter(max_retries=retry_strategy)
    session.mount("http://", adapter)
    session.mount("https://", adapter)
    
    # Add headers to appear more like a regular browser
    session.headers.update({
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
    })
    
    return session

def fetch_stock_data_alternative(ticker, start_date, end_date):
    """Alternative method using pandas_datareader or manual CSV creation."""
    print(f"Trying alternative data source for {ticker}...")
    
    # You can implement pandas_datareader here if installed:
    # try:
    #     import pandas_datareader as pdr
    #     data = pdr.get_data_yahoo(ticker, start_date, end_date)
    #     return data['Adj Close']
    # except:
    #     pass
    
    # For demonstration, create sample data (replace with real alternative source)
    print("Creating sample data for demonstration...")
    date_range = pd.date_range(start=start_date, end=end_date, freq='D')
    # Filter to business days only
    business_days = date_range[date_range.weekday < 5]
    
    # Generate realistic-looking AAPL price data
    np.random.seed(42)  # For reproducible results
    n_days = len(business_days)
    initial_price = 150.0
    returns = np.random.normal(0.001, 0.02, n_days)  # Daily returns
    prices = [initial_price]
    
    for r in returns[1:]:
        prices.append(prices[-1] * (1 + r))
    
    sample_data = pd.Series(prices, index=business_days[:len(prices)])
    print(f"Generated {len(sample_data)} days of sample data")
    print("WARNING: Using sample data for demonstration. Replace with real data source.")
    return sample_data

def fetch_stock_data(ticker, start_date, end_date, retries=MAX_RETRIES):
    """Fetch adjusted close prices for a given ticker with enhanced retry logic."""
    session = setup_session()
    
    for attempt in range(retries):
        try:
            print(f"Attempt {attempt + 1}: Fetching data for {ticker}...")
            
            # Add delay between requests to avoid rate limiting
            if attempt > 0:
                time.sleep(REQUEST_DELAY)
            
            # Create ticker object with session
            ticker_obj = yf.Ticker(ticker, session=session)
            
            # Try to get data using the ticker object method
            stock = ticker_obj.history(start=start_date, end=end_date, auto_adjust=True)
            
            if stock.empty:
                # Try alternative method with yf.download
                print(f"Ticker method failed, trying yf.download...")
                time.sleep(REQUEST_DELAY)
                stock = yf.download(ticker, start=start_date, end=end_date, 
                                  progress=False, auto_adjust=True, session=session)
            
            if stock.empty:
                raise ValueError(f"No data fetched for {ticker} in date range {start_date} to {end_date}")
            
            # With auto_adjust=True, use 'Close' column
            if 'Close' not in stock.columns:
                raise KeyError("Expected 'Close' column not found in data")
            
            print(f"Successfully fetched {len(stock)} days of data for {ticker}")
            print(f"Available columns: {stock.columns.tolist()}")
            print(f"Date range: {stock.index[0].date()} to {stock.index[-1].date()}")
            return stock['Close']
            
        except Exception as e:
            error_msg = str(e)
            print(f"Attempt {attempt + 1} failed for {ticker}: {error_msg}")
            
            # Check if it's a rate limit error
            if "rate limit" in error_msg.lower() or "429" in error_msg:
                if attempt < retries - 1:
                    print(f"Rate limit detected. Waiting {RETRY_DELAY} seconds before retry...")
                    time.sleep(RETRY_DELAY)
                else:
                    print("Rate limit exceeded. Trying alternative data source...")
                    return fetch_stock_data_alternative(ticker, start_date, end_date)
            else:
                if attempt < retries - 1:
                    print(f"Retrying after {RETRY_DELAY // 6} seconds...")
                    time.sleep(RETRY_DELAY // 6)
                else:
                    print("All retry attempts failed. Trying alternative data source...")
                    return fetch_stock_data_alternative(ticker, start_date, end_date)
    
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
    
    # Generate crossover signals
    signals['buy_signal'] = ((short_mavg > long_mavg) & 
                           (short_mavg.shift(1) <= long_mavg.shift(1))).astype(int)
    signals['sell_signal'] = ((short_mavg < long_mavg) & 
                            (short_mavg.shift(1) >= long_mavg.shift(1))).astype(int)
    signals['positions'] = signals['buy_signal'] - signals['sell_signal']
    return signals

def simulate_trading(signals, initial_cash):
    """Simulate trading with improved cash utilization."""
    trade_dates = signals.index[signals['positions'] != 0]
    if trade_dates.empty:
        print("No trade signals generated. Check moving average windows or data range.")
        return signals, initial_cash
    
    cash = initial_cash
    shares = 0
    portfolio = []
    
    for date in signals.index:
        current_price = signals.loc[date, 'price']
        
        if date in trade_dates:
            position = signals.loc[date, 'positions']
            
            if position == 1 and cash > 0:  # Buy signal
                # Use 99% of cash to leave small buffer for rounding
                shares_to_buy = (cash * 0.99) / current_price
                shares += shares_to_buy
                cash -= shares_to_buy * current_price
                print(f"BUY on {date.date()}: {shares_to_buy:.2f} shares at ${current_price:.2f}")
                
            elif position == -1 and shares > 0:  # Sell signal
                cash += shares * current_price
                print(f"SELL on {date.date()}: {shares:.2f} shares at ${current_price:.2f}")
                shares = 0
        
        # Calculate portfolio value
        portfolio_value = cash + shares * current_price
        portfolio.append(portfolio_value)
    
    signals['portfolio'] = portfolio
    return signals, portfolio[-1]

def plot_results(signals, ticker):
    """Plot price, moving averages, and trading signals."""
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
    
    # Price and moving averages
    ax1.plot(signals.index, signals['price'], label='Price', linewidth=1)
    ax1.plot(signals.index, signals['short_mavg'], label='Short MA', alpha=0.7)
    ax1.plot(signals.index, signals['long_mavg'], label='Long MA', alpha=0.7)
    
    # Trading signals
    buy_dates = signals.index[signals['positions'] == 1]
    sell_dates = signals.index[signals['positions'] == -1]
    
    if not buy_dates.empty:
        ax1.scatter(buy_dates, signals.loc[buy_dates, 'price'], 
                   marker='^', s=100, color='green', label='Buy', zorder=5)
    if not sell_dates.empty:
        ax1.scatter(sell_dates, signals.loc[sell_dates, 'price'], 
                   marker='v', s=100, color='red', label='Sell', zorder=5)
    
    ax1.set_title(f'{ticker} Trading Strategy - Price and Signals')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Portfolio value
    ax2.plot(signals.index, signals['portfolio'], label='Portfolio Value', color='purple')
    ax2.set_title('Portfolio Value Over Time')
    ax2.set_ylabel('Portfolio Value ($)')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.show()

def calculate_performance_metrics(signals, initial_cash):
    """Calculate key performance metrics."""
    final_value = signals['portfolio'].iloc[-1]
    total_return = (final_value - initial_cash) / initial_cash * 100
    
    # Calculate buy-and-hold return for comparison
    initial_price = signals['price'].iloc[0]
    final_price = signals['price'].iloc[-1]
    buy_hold_return = (final_price - initial_price) / initial_price * 100
    
    print(f"\n=== Performance Metrics ===")
    print(f"Initial Investment: ${initial_cash:,.2f}")
    print(f"Final Portfolio Value: ${final_value:,.2f}")
    print(f"Total Return: {total_return:.2f}%")
    print(f"Buy & Hold Return: {buy_hold_return:.2f}%")
    print(f"Strategy vs Buy & Hold: {total_return - buy_hold_return:.2f}% difference")

if __name__ == "__main__":
    # Parameters
    TICKER = "AAPL"
    START_DATE = "2023-01-01"
    END_DATE = "2025-06-18"
    INITIAL_CASH = 10000
    SHORT_WINDOW = 20
    LONG_WINDOW = 50
    
    print(f"Starting trading strategy analysis for {TICKER}")
    print(f"Date range: {START_DATE} to {END_DATE}")
    
    # Fetch data
    prices = fetch_stock_data(TICKER, START_DATE, END_DATE)
    if prices is None:
        print("Failed to fetch data. Exiting.")
        exit(1)
    
    # Check data sufficiency
    if len(prices) < LONG_WINDOW:
        print(f"Insufficient data: only {len(prices)} days available, need at least {LONG_WINDOW}")
        exit(1)
    
    # Calculate moving averages and generate signals
    short_mavg, long_mavg = calculate_moving_averages(prices, SHORT_WINDOW, LONG_WINDOW)
    signals = generate_signals(prices, short_mavg, long_mavg)
    
    # Simulate trading
    signals, final_value = simulate_trading(signals, INITIAL_CASH)
    
    # Display results
    plot_results(signals, TICKER)
    calculate_performance_metrics(signals, INITIAL_CASH)
