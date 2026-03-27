// SPDX-License-Identifier: GPL-3.0-or-later
pragma solidity ^0.8.0;

/// @title WETH9 — Wrapped Ether
/// @notice Wraps native ETH into an ERC-20-compatible token so it can be used
///         wherever ERC-20 tokens are accepted.  Each WETH token is backed 1:1
///         by ETH held in this contract.
contract WETH9 {
    string public name     = "Wrapped Ether";
    string public symbol   = "WETH";
    uint8  public decimals = 18;

    event Approval(address indexed src, address indexed guy, uint wad);
    event Transfer(address indexed src, address indexed dst, uint wad);
    event Deposit(address indexed dst, uint wad);
    event Withdrawal(address indexed src, uint wad);

    mapping(address => uint)                       public balanceOf;
    mapping(address => mapping(address => uint))   public allowance;

    /// @notice Wrap ETH — send ETH to this function and receive an equal
    ///         amount of WETH in return.
    receive() external payable {
        deposit();
    }

    /// @notice Wrap ETH — the same as sending ETH directly to the contract.
    function deposit() public payable {
        balanceOf[msg.sender] += msg.value;
        emit Deposit(msg.sender, msg.value);
    }

    /// @notice Unwrap WETH — burn `wad` WETH and receive an equal amount of ETH.
    /// @param wad Amount of WETH (in wei) to unwrap.
    function withdraw(uint wad) public {
        require(balanceOf[msg.sender] >= wad, "WETH: insufficient balance");
        balanceOf[msg.sender] -= wad;
        emit Withdrawal(msg.sender, wad);
        (bool success, ) = payable(msg.sender).call{value: wad}("");
        require(success, "WETH: ETH transfer failed");
    }

    /// @notice Returns the total amount of ETH (in wei) held by this contract,
    ///         which equals the total supply of WETH tokens in circulation.
    function totalSupply() public view returns (uint) {
        return address(this).balance;
    }

    /// @notice Approve `guy` to spend up to `wad` of the caller's WETH.
    /// @param guy  The spender address.
    /// @param wad  Allowance amount in wei.
    function approve(address guy, uint wad) public returns (bool) {
        allowance[msg.sender][guy] = wad;
        emit Approval(msg.sender, guy, wad);
        return true;
    }

    /// @notice Transfer `wad` WETH from the caller to `dst`.
    /// @param dst  Recipient address.
    /// @param wad  Amount in wei.
    function transfer(address dst, uint wad) public returns (bool) {
        return transferFrom(msg.sender, dst, wad);
    }

    /// @notice Transfer `wad` WETH from `src` to `dst`, consuming allowance if
    ///         the caller is not `src`.
    /// @param src  Source address.
    /// @param dst  Destination address.
    /// @param wad  Amount in wei.
    function transferFrom(address src, address dst, uint wad)
        public
        returns (bool)
    {
        require(dst != address(0), "WETH: transfer to zero address");
        require(balanceOf[src] >= wad, "WETH: insufficient balance");

        if (src != msg.sender && allowance[src][msg.sender] != type(uint).max) {
            require(allowance[src][msg.sender] >= wad, "WETH: allowance exceeded");
            allowance[src][msg.sender] -= wad;
        }

        balanceOf[src] -= wad;
        balanceOf[dst] += wad;

        emit Transfer(src, dst, wad);
        return true;
    }
}
